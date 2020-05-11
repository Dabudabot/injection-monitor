@echo off

call rundll32 syssetup,SetupInfObjectInstallAction DefaultInstall 128 .\imdrv.inf

sc start imdrv

start /wait imapp.exe

sc stop imdrv

sc delete imdrv