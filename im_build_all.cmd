@echo off

if "%1"=="" goto :fail
if "%2"=="" goto :fail
if "%3"=="" goto :fail

for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"

set "datestamp=%YYYY%%MM%%DD%" & set "timestamp=%HH%%Min%%Sec%"

set MACHINE=%1%
set SNAPSHOT=%2%
set SHARE_FOLDER=%3\build\%datestamp%%timestamp%

REM call "C:\Program Files (x86)\VMware\VMware VIX\vmrun.exe" revertToSnapshot %MACHINE% %SNAPSHOT%
REM call "C:\Program Files (x86)\VMware\VMware VIX\vmrun.exe" start %MACHINE%

call im_build.cmd debug Win32 Windows10 %SHARE_FOLDER%
call im_build.cmd debug x64 Windows10 %SHARE_FOLDER%
call im_build.cmd debug Win32 Windows7 %SHARE_FOLDER%
call im_build.cmd debug x64 Windows7 %SHARE_FOLDER%

copy /Y "deploy.cmd" %SHARE_FOLDER%
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