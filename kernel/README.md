# Injector monitor file system minifilter driver

### imdrv.sys

Driver performs the following:
1. In driver entry (im_drv.c) we initialize global variables, such as NPagedLookasideList where we are going to keep records, we define target process names, configure communication port using FltCreateCommunicationPort, register callback on process creation (or deletion) using PsSetCreateProcessNotifyRoutine and finally start our filter
2. Communication (im_comm.c) in current version is just one command to get records from the list. We just copy record and strings to fill avaliable memory or send STATUS_NO_MORE_ENTRIES, if there are no records.
3. As soon as new process appears or deleted our function is triggered (im_proc.c). It check is this process name (using ZwQueryInformationProcess) in our target processes array. If so, we save this process ID and path to image. If target process was killed we forget it`s id.
4. Filter is registered on IRM_MJ_CREATE and we have pre and post callback (im_ops.c). We ignore paging files or opening by fileid, if process that tries to open file is our target process (checking by process id) we request file name using FltGetFileNameInformation and make decision about reparsing load.
5. By requirements we have to block loading sw.dll and load hw.dll instead. So in our decision if filename is equal to sw.dll we finishing this IRP with STATUS_REPARSE. In reparse information we put path to hw.dll.
6. If it is our process, name was retrieved and we are opening file with EXECURE rights we create record to log this event.
7. In post callback we make decision should we block loading or not. We are checking by requirements file and if it has to be blocked we just call FltCancelFileOpen. Everything is logged to the record and collected to the list.

## Build

Visual Studio 2019 (Version 16.5.5) (SDK 10.0.19041.0 or change to latest), WDK 10 (10.0.19030.1000)

---------------------------------------
Tested on Windows 7 x64, builds for Windows 7 x86 x64, Windows 10 x86 x64