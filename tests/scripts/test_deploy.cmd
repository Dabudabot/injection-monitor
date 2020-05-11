@echo off

set GAME_FOLDER=%1%

copy /y hl.exe %GAME_FOLDER%
copy /y testdll0.dll %GAME_FOLDER%
copy /y msvcp120d.dll %GAME_FOLDER%
copy /y msvcr120d.dll %GAME_FOLDER%
REM SET PATH=%PATH%;%cd%

echo Ready to start test

timeout /t 15
REM pause

call %GAME_FOLDER%\hl.exe