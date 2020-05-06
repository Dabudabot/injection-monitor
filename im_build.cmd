@echo off

if "%1"=="" goto :fail
if "%2"=="" goto :fail
if "%3"=="" goto :fail

set MACHINE=%1%
set SNAPSHOT=%2%
set SHARE_FOLDER=%3%

call msbuildex.cmd "kernel\imdrv" imdrv debug x64 "libs/imlib" imlib debug x64 "client\imapp" imapp debug x64 "tests\testlib0" testlib0 debug x64 "tests\testlib1" testlib1 debug x64 "tests\testapp" testapp debug x64

if errorlevel 1 goto :fail

REM copy to the share all what we need to test

call "C:\Program Files (x86)\VMware\VMware VIX\vmrun.exe" revertToSnapshot %MACHINE% %SNAPSHOT%
call "C:\Program Files (x86)\VMware\VMware VIX\vmrun.exe" start %MACHINE%

copy /Y "build\x64\drv\imdrv.inf" %SHARE_FOLDER%
copy /Y "build\x64\drv\imdrv.sys" %SHARE_FOLDER%
copy /Y "build\x64\drv\imdrv.pdb" %SHARE_FOLDER%

copy /Y "build\x64\exe\imapp.exe" %SHARE_FOLDER%
copy /Y "build\x64\exe\imapp.pdb" %SHARE_FOLDER%

copy /Y "build\x64\exe\hl.exe" %SHARE_FOLDER%
copy /Y "build\x64\exe\hl.pdb" %SHARE_FOLDER%

copy /Y "build\x64\lib\testlib0.dll" %SHARE_FOLDER%
copy /Y "build\x64\lib\testlib0.pdb" %SHARE_FOLDER%
copy /Y "build\x64\lib\testlib1.dll" %SHARE_FOLDER%
copy /Y "build\x64\lib\testlib1.pdb" %SHARE_FOLDER%

copy /Y "im_deploy.cmd" %SHARE_FOLDER%
copy /Y "tests\scripts\test_deploy.cmd" %SHARE_FOLDER%

echo *
echo ********************************************
echo DONE
echo ********************************************
echo *
goto :end
:fail
echo *
echo ********************************************
echo FAILED
echo ********************************************
echo *
:end