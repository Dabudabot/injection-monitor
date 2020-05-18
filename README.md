# Injector monitor file system minifilter driver

### imdrv.sys

Driver registered on IRP_MJ_CREATE. Driver checks SecurityContext on pre create callback in case of FILE_EXECUTE, it proceeds to post create. Driver collects process name of the caller using ZwQueryInformationProcess. In case if process name equals to hardcoded names driver collects file name (file that this process tries to open with execute rights). In case if file satisfies requirements driver just log information, otherwise cancel this IRP with ACCESS_DENIED error and log information.

### imlib.lib

Library provides communication with driver to callect logs.

### imapp.exe

User mode app just an example of usage of the library.

### testapp and testdll

Tests are simple helloworld examples to test library load.

## Build

1. Install Visual Studio 2019 (Version 16.5.5) (SDK 10.0.19041.0 or change to latest), WDK 10 (10.0.19030.1000)
2. Add path to msbuild (for example: C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64) to path environment variable
3. run im_build_all.cmd (it requires 3 parameters: path to VM, snapshot name, path to shared with vm folder). Change this script if you use other virtual machine than vmware.
4. binaries is in build directory and copied to share folder

## Deploy

To deploy driver and application run im_deploy.cmd in virtual machine

## Further development:

1. Do not hardcode requirements of accepting or declining (need to recieve them via IOCTL) or other way
2. Implement dll-injector to test on injections

## Demo:

![demo1](docs/demo1.gif)
REPARSING DLL LOAD

![demo2](docs/demo2.gif)
RESTRICTED LOAD FROM DIRECTORIES

![demo3](docs/demo3.gif)
DEMO ON GAME

---------------------------------------
Tested on Windows 7 x64, builds for Windows 7 x86 x64, Windows 10 x86 x64