#include <stdio.h>
#include <Mmdeviceapi.h>
#include <audioclient.h>
#include "main.h"

// Record an audio stream from the default audio capture
// device. Poll the peak intensity every 1/10 second and
// toggle the specified key when the intensity exceeds a
// threshold. Based on the Microsoft stream capture example.
// https://msdn.microsoft.com/en-us/library/dd370800%28v=vs.85%29.aspx

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define EXIT_ON_ERROR(hres) \
if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
if ((punk) != NULL) { (punk)->lpVtbl->Release(punk); (punk) = NULL; }

static const GUID CLSID_MMDeviceEnumerator = { 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E };
static const GUID IID_IMMDeviceEnumerator = { 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 };
static const GUID IID_IAudioClient = { 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 };
static const GUID IID_IAudioCaptureClient = { 0xC8ADBD64, 0xE71E, 0x48A0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17 };

HRESULT RecordAudioStream(/*MyAudioSink *pMySink*/)
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC / 5;
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

	// Notify the audio sink which format to use.
	/*hr = pMySink->SetFormat(pwfx);
	EXIT_ON_ERROR(hr)*/

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

		while (packetLength != 0)
		{
			// Get the available data in the shared buffer.
			hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
			EXIT_ON_ERROR(hr)

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				pData = NULL;  // Tell CopyData to write silence.
			}

			// Copy the available capture data to the audio sink.
			if (pData == NULL) printf("Null data");
			else printf("%02x\n", pData);

			bDone = TRUE; //hr = pMySink->CopyData(pData, numFramesAvailable, &bDone);
			EXIT_ON_ERROR(hr)

			hr = pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
			EXIT_ON_ERROR(hr)

			hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
			EXIT_ON_ERROR(hr)
		}
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
	RecordAudioStream();
	getchar();
	return 0;
}
