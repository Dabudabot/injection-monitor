@echo off

if "%1"=="" goto :fail
if "%2"=="" goto :fail
if "%3"=="" goto :fail
if "%4"=="" goto :fail

set T_CONF=%1%
set T_PLAT=%2%
set T_OS=%3%
set SHARE_BUILD_FOLDER=%4%

call msbuildex.cmd "kernel\imdrv" imdrv %T_CONF% %T_PLAT% %T_OS% "libs/imlib" imlib %T_CONF% %T_PLAT% %T_OS% "client\imapp" imapp %T_CONF% %T_PLAT% %T_OS% "tests\testdll0" testdll0 %T_CONF% %T_PLAT% %T_OS% "tests\testdll1" testdll1 %T_CONF% %T_PLAT% %T_OS% "tests\testapp0" testapp0 %T_CONF% %T_PLAT% %T_OS%

if errorlevel 1 goto :fail

REM copy to the share all what we need to test

if not exist %SHARE_BUILD_FOLDER% mkdir %SHARE_BUILD_FOLDER%
mkdir %SHARE_BUILD_FOLDER%\%T_OS%\%T_PLAT%\%T_CONF%

set THIS_BUILD_FOLDER=%SHARE_BUILD_FOLDER%\%T_OS%\%T_PLAT%\%T_CONF%

echo %SHARE_BUILD_FOLDER%
echo %THIS_BUILD_FOLDER%
echo "build\%T_OS%\%T_PLAT%\%T_CONF%\drv\imdrv.inf"

copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\drv\imdrv.inf" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\drv\imdrv.sys" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\drv\imdrv.pdb" %THIS_BUILD_FOLDER%

copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\exe\imapp.exe" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\exe\imapp.pdb" %THIS_BUILD_FOLDER%

copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\exe\hl.exe" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\exe\hl.pdb" %THIS_BUILD_FOLDER%

copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\lib\testdll0.dll" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\lib\testdll0.pdb" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\lib\testdll1.dll" %THIS_BUILD_FOLDER%
copy /Y "build\%T_OS%\%T_PLAT%\%T_CONF%\lib\testdll1.pdb" %THIS_BUILD_FOLDER%

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