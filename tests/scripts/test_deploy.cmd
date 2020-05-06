@echo off

set GAME_FOLDER=%1%

copy /y hl.exe %GAME_FOLDER%
copy /y testlib0.dll %GAME_FOLDER%
SET PATH=%PATH%;%cd%

echo Ready to start test

pause

call %GAME_FOLDER%\hl.exe