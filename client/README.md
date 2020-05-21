# Injector monitor client application

### imapp.exe

User mode app just an example of usage of the library. It has to:
1. Initilize library and provide callback for it
2. Callback receives record use it. It will be freed later in library
3. Deinitilize library

## Run

IMPORTANT: you need admin privilages to run this application (cause library performs DeviceIOControl which requires admin rights)

---------------------------------------
Tested on Windows 7 x64, builds for Windows 7 x86 x64, Windows 10 x86 x64