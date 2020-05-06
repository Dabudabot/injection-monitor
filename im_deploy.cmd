@echo off

call rundll32 syssetup,SetupInfObjectInstallAction DefaultInstall 128 .\imdrv.inf

REM TODO start driver

call imapp.exe