@echo off

set CURRENT_DIRECTORY=%cd%
if "%1"=="" goto :error

:check
if "%1"=="" goto :end

set PATH_TO_PROJECT=%1%
shift
if "%1"=="" goto :error
set PROJECT_NAME=%1%
shift
if "%1"=="" goto :error
set CONFIG=%1%
shift
if "%1"=="" goto :error
set PLATFORM=%1%
shift
if "%1"=="" goto :error
set OS=%1%
shift

cd %PATH_TO_PROJECT%

echo -
echo ---------------------------------------------------------------------
echo STARTING: Building %PROJECT_NAME%              
echo ---------------------------------------------------------------------
echo -

call msbuild %PROJECT_NAME%.vcxproj /p:configuration=%CONFIG% /p:platform=%PLATFORM% /p:TargetVersion=%OS%

if errorlevel 1 goto :error

cd %CURRENT_DIRECTORY%

echo -
echo ---------------------------------------------------------------------
echo SUCCESS: Building %PROJECT_NAME%              
echo ---------------------------------------------------------------------
echo -

goto :check

:error
cd %CURRENT_DIRECTORY%
echo *
echo ********************************************
echo ERROR msbuildex.cmd
echo ********************************************
echo *
EXIT /B 1

:end
echo -
echo ---------------------------------------------------------------------
echo SUCCESS: Building win projects             
echo ---------------------------------------------------------------------
echo -
EXIT /B 0