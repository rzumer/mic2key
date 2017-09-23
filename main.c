#include <stdio.h>
#include <Mmdeviceapi.h>
#include <audioclient.h>

// Process bytes and return the peak amplitude normalized on a scale of 100.
int GetAmplitude(WAVEFORMATEX *format, BYTE *data, UINT32 *numFrames)
{
	/*printf("%u\n", *numFrames);
	printf("%u\n", sizeof(*data));
	printf("%u\n", format->wBitsPerSample);
	printf("%u\n", format->nChannels);
	printf("%u\n", format->cbSize);*/
	/*
	switch (format->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
		printf("pcm");
		break;
	case WAVE_FORMAT_EXTENSIBLE:
		printf("extensible");
		break;
	case WAVE_FORMAT_MPEG:
		printf("mpeg");
		break;
	case WAVE_FORMAT_MPEGLAYER3:
		printf("mp3");
		break;
	}*/

	return 0;
}

// Record an audio stream from the default audio capture
// device. Poll the peak intensity every 1/10 second and
// toggle the specified key when the intensity exceeds a
// threshold. Based on the Microsoft stream capture example.
// https://msdn.microsoft.com/en-us/library/dd370800%28v=vs.85%29.aspx

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

// Maximum buffer length in milliseconds
#define MAX_BUFFER_LENGTH 10000

// Minimm and maximum values for the threshold parameter
#define MIN_THRESHOLD_VALUE 0
#define MAX_THRESHOLD_VALUE 100

#define MAX(a,b) ((a) > (b) ? a : b)

#define EXIT_ON_ERROR(hres) \
if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
if ((punk) != NULL) { (punk)->lpVtbl->Release(punk); (punk) = NULL; }

static const GUID CLSID_MMDeviceEnumerator = { 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E };
static const GUID IID_IMMDeviceEnumerator = { 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 };
static const GUID IID_IAudioClient = { 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 };
static const GUID IID_IAudioCaptureClient = { 0xC8ADBD64, 0xE71E, 0x48A0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17 };

HRESULT RecordAudioStream(int timeUnit, int threshold, WORD keyCode)
{
	if (timeUnit <= 0 || timeUnit > MAX_BUFFER_LENGTH)
	{
		printf("Recorded time unit is out of permitted range.");
		return E_INVALIDARG;
	}
	if (threshold < MIN_THRESHOLD_VALUE || threshold > MAX_THRESHOLD_VALUE)
	{
		printf("Threshold is out of range.");
		return E_INVALIDARG;
	}

	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_MILLISEC * timeUnit;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	UINT32 packetLength = 0;
	BOOL bDone = FALSE;
	BYTE *pData;
	DWORD flags;
	int peak = 0;
	INPUT input = { .type = INPUT_KEYBOARD,
		.ki.wScan = MapVirtualKey(keyCode, MAPVK_VK_TO_VSC),
		.ki.time = 0,
		.ki.dwExtraInfo = 0,
		.ki.wVk = keyCode,
		.ki.dwFlags = 0 };

	CoInitialize(0);

	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**) &pEnumerator);
	EXIT_ON_ERROR(hr)

	hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eCapture, eCommunications, &pDevice);
	EXIT_ON_ERROR(hr)

	hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

	hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
	EXIT_ON_ERROR(hr)

	hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, pwfx, NULL);
	EXIT_ON_ERROR(hr)

	// Get the size of the allocated buffer.
	hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount);
	EXIT_ON_ERROR(hr)

	hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient, (void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)

	// Calculate the actual duration of the allocated buffer.
	hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->lpVtbl->Start(pAudioClient);  // Start recording.
	EXIT_ON_ERROR(hr)

	// Each loop fills about half of the shared buffer.
	while (bDone == FALSE)
	{
		// Sleep for half the buffer duration.
		Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

		hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
		EXIT_ON_ERROR(hr)

		// Reset the peak value.
		peak = 0;

		while (packetLength != 0)
		{
			// Get the available data in the shared buffer.
			hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
			EXIT_ON_ERROR(hr)

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				pData = 0;
			}

			// Concatenate bytes
			//if (pData == NULL) printf("Null data");
			//else printf("%02x\n", pData);

			int amplitude = GetAmplitude(pwfx, pData, &numFramesAvailable);
			peak = MAX(peak, amplitude);

			hr = pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
			EXIT_ON_ERROR(hr)

			hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
			EXIT_ON_ERROR(hr)
		}

		if (peak >= threshold)
		{
			printf("Y\n");
			input.ki.dwFlags = 0;
		}
		else
		{
			printf("N\n");
			input.ki.dwFlags = KEYEVENTF_KEYUP;
		}

		// Send input (key down or up depending on peak value).
		SendInput(1, &input, sizeof(INPUT));

		bDone = TRUE;
	}

	hr = pAudioClient->lpVtbl->Stop(pAudioClient);  // Stop recording.
	EXIT_ON_ERROR(hr)
		
Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
	SAFE_RELEASE(pDevice)
	SAFE_RELEASE(pAudioClient)
	SAFE_RELEASE(pCaptureClient)

	return hr;
}

int main() {
	RecordAudioStream(100, 50, VK_SPACE);
	getchar();
	return 0;
}
