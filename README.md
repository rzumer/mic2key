# mic2key
Bind microphone input to the keyboard.  
Set an interval, threshold value and virtual key code to trigger. The peak level recorded by the microphone over the specified interval is compared with the threshold in order to determine whether the key should be toggled or not.

Requires Windows Vista or later due to reliance on Windows APIs (`WASAPI`).  
Only 32-bit float audio input is currently supported. Only the first channel is processed on multi-channel recording devices.

Usage: `mic2key <interval> <threshold>`

Project files are generated with Visual Studio 2017.
