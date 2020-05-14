/*++

author:

Daulet Tumbayev

Module Name:

im_ops.c

Abstract:
filtering callbacks

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im_ops.h"
#include "im_req.h"
#include "im_utils.h"
#include "im_rec.h"
#include "im_list.h"

//------------------------------------------------------------------------
//  Defines.
//------------------------------------------------------------------------

#define IM_ALLOWED_EXTENTION L"dll"

#define IM_RESTRICTED_FILE L"Steam\\crashhandler.dll" //consider to not to hardcode it

#define IM_ALLOWED_DIR_1 L"\\Device\\HarddiskVolume3\\Windows\\" // todo look for right device harddisk

#define IM_SW_DLL L"sw.dll"
#define IM_HW_DLL L"hw.dll"

//------------------------------------------------------------------------
//  Local functions
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMDecideVideoMode(
        _In_ PFILE_OBJECT FileObject,
        _In_ PIM_NAME_INFORMATION ProcessNameInfo,
        _In_ PIM_NAME_INFORMATION FileNameInfo,
        _Out_ PIM_VIDEO_MODE_STATUS VideoMode);

_Check_return_
    NTSTATUS
    IMDecideBlock(
        _In_ PUNICODE_STRING FullProcessName,
        _In_ PIM_NAME_INFORMATION FileNameInfo,
        _Out_ PBOOLEAN IsBlocked);

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMPreCreate)
#pragma alloc_text(PAGE, IMPostCreate)
#pragma alloc_text(PAGE, IMDecideVideoMode)
#pragma alloc_text(PAGE, IMDecideBlock)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

FLT_PREOP_CALLBACK_STATUS
FLTAPI
IMPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext)
{
  ACCESS_MASK desiredAccess;
  FLT_PREOP_CALLBACK_STATUS cbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
  NTSTATUS status = STATUS_SUCCESS;
  ULONG i = 0;
  HANDLE processId = NULL;
  PIM_NAME_INFORMATION processNameInfo = NULL;
  PIM_NAME_INFORMATION fileNameInfo = NULL;
  PIM_KRECORD_LIST recordList = NULL;
  IM_VIDEO_MODE_STATUS videoMode = IM_NOT_APPLICABLE;

  *CompletionContext = NULL;

  PAGED_CODE();

  UNREFERENCED_PARAMETER(FltObjects);

  // We are only registered for the IRP_MJ_CREATE.
  FLT_ASSERT(Data != NULL);
  FLT_ASSERT(Data->Iopb != NULL);
  FLT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_CREATE);

  __try
  {
    desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
    processId = PsGetCurrentProcessId();

    // we skip paging
    // we not looking for volumes
    // we do not work in opening by id
    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE) || FlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN))
    {
      __leave;
    }

    // is current process id is our process?
    for (; i < IM_AMOUNT_OF_TARGET_PROCESSES; i++)
    {
      if (processId == Globals.TargetProcessInfo[i].ProcessId && Globals.TargetProcessInfo[i].isActive)
      {
        processNameInfo = Globals.TargetProcessInfo[i].NameInfo;
      }
    }

    // it is not our target process
    if (NULL == processNameInfo)
    {
      __leave;
    }
    else
    {
      LOG(("[IM] We are working now with %wZ\n", &processNameInfo->Name));
    }

    // get file info of the file witch are opening by the process
    NT_IF_FAIL_LEAVE(IMGetFileNameInformation(Data, &fileNameInfo));

    // now we make decision about video mode
    NT_IF_FAIL_LEAVE(IMDecideVideoMode(Data->Iopb->TargetFileObject, processNameInfo, fileNameInfo, &videoMode));

    // opening without execution rights, not our case but if we speak about video mode we have to log all
    // cause in case of applicable it is hw.dll or sw.dll
    if (!FlagOn(desiredAccess, FILE_EXECUTE) && videoMode == IM_NOT_APPLICABLE)
    {
      __leave;
    }

    // now we create record for log
    NT_IF_FAIL_LEAVE(IMCreateRecord(&recordList, Data, fileNameInfo, &processNameInfo->FullName, videoMode));
  }
  __finally
  {
    if (fileNameInfo != NULL && (recordList == NULL || IM_VIDEO_HW_TO_SW == videoMode || IM_VIDEO_SW_TO_HW == videoMode))
    {
      IMReleaseNameInformation(fileNameInfo);
    }

    if (NT_SUCCESS(status))
    {
      // if need to change mode we fake reparse point
      if (IM_VIDEO_HW_TO_SW == videoMode || IM_VIDEO_SW_TO_HW == videoMode)
      {
        Data->IoStatus.Status = STATUS_REPARSE;
        Data->IoStatus.Information = IO_REPARSE;
        cbStatus = FLT_PREOP_COMPLETE;
        recordList->Record.IsSucceded = TRUE;
        IMPush(&recordList->List, &Globals.RecordsHead);
      }
      else
      {
        if (NULL != recordList)
        {
          *CompletionContext = recordList;
          cbStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
        else
        {
          *CompletionContext = NULL;
          cbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
      }
    }
    else
    {
      // thats should not happen
      Data->IoStatus.Status = status;
      cbStatus = FLT_PREOP_COMPLETE;
    }
  }

  return cbStatus;
}

FLT_POSTOP_CALLBACK_STATUS
FLTAPI
IMPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags)
{
  NTSTATUS status = STATUS_SUCCESS;
  PIM_KRECORD_LIST recordList = NULL;
  PIM_NAME_INFORMATION fileNameInfo = NULL;
  BOOLEAN isBlocked = FALSE;
  UNICODE_STRING fullProcessName;

  UNREFERENCED_PARAMETER(Flags);
  UNREFERENCED_PARAMETER(CompletionContext);

  // We are only registered for the IRP_MJ_CREATE.
  FLT_ASSERT(Data != NULL);
  FLT_ASSERT(Data->Iopb != NULL);
  FLT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_CREATE);
  FLT_ASSERT(CompletionContext != NULL);

  RtlZeroMemory(&fullProcessName, sizeof(UNICODE_STRING));
  recordList = (PIM_KRECORD_LIST)CompletionContext;

  LOG(("[IM] Post create start\n"));

  __try
  {
    fileNameInfo = (PIM_NAME_INFORMATION)recordList->Record.FileNameInformation;

    if (NULL == fileNameInfo)
    {
      status = STATUS_UNSUCCESSFUL;
      __leave;
    }

    NT_IF_FAIL_LEAVE(IMToString((PWCHAR)recordList->Record.Data[IM_PROCESS_NAME_INDEX].Buffer, recordList->Record.Data[IM_PROCESS_NAME_INDEX].Size, &fullProcessName));

    //
    // now we deciding to block load or not
    //
    NT_IF_FAIL_LEAVE(IMDecideBlock(&fullProcessName, fileNameInfo, &isBlocked));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Operatin failed\n"));
    }

    if (NULL != fullProcessName.Buffer)
    {
      ExFreePool(fullProcessName.Buffer);
    }

    if (NULL != fileNameInfo)
    {
      IMReleaseNameInformation(fileNameInfo);
    }

    if (isBlocked)
    {
      FltCancelFileOpen(FltObjects->Instance, FltObjects->FileObject);
      Data->IoStatus.Status = STATUS_ACCESS_DENIED;
      Data->IoStatus.Information = 0;
      LOG(("[IM] Loading blocked\n"));
    }

    if (NULL != recordList)
    {
      recordList->Record.IsBlocked = isBlocked;
      if (NT_SUCCESS(status))
      {
        LOG(("[IM] Operation succeeded\n"));
        recordList->Record.IsSucceded = TRUE;
      }
      IMPush(&recordList->List, &Globals.RecordsHead);
    }
  }

  return FLT_POSTOP_FINISHED_PROCESSING;
}

_Check_return_
    NTSTATUS
    IMDecideVideoMode(
        _In_ PFILE_OBJECT FileObject,
        _In_ PIM_NAME_INFORMATION ProcessNameInfo,
        _In_ PIM_NAME_INFORMATION FileNameInfo,
        _Out_ PIM_VIDEO_MODE_STATUS VideoMode)
{
  NTSTATUS status = STATUS_SUCCESS;
  IM_VIDEO_MODE_STATUS videoMode = IM_NOT_APPLICABLE;
  UNICODE_STRING strSw = CONSTANT_STRING(IM_SW_DLL);
  UNICODE_STRING strHw = CONSTANT_STRING(IM_HW_DLL);
  UNICODE_STRING replacement;

  PAGED_CODE();

  RtlZeroMemory(&replacement, sizeof(UNICODE_STRING));
  *VideoMode = IM_NOT_APPLICABLE;

  __try
  {
    // it is only works with hl
    if (RtlCompareUnicodeString(&ProcessNameInfo->Name, &Globals.TargetProcessInfo[IM_HL_PROCESS_INFO_INDEX].TargetName, TRUE) != 0)
    {
      __leave;
    }

    // it is only works with files from game folder
    if (RtlCompareUnicodeString(&ProcessNameInfo->ParentDir, &FileNameInfo->ParentDir, TRUE) != 0)
    {
      __leave;
    }

    // is it already hw?
    if (RtlCompareUnicodeString(&FileNameInfo->Name, &strHw, TRUE) == 0)
    {
      videoMode = IM_VIDEO_HW;
      __leave;
    }

    // may be it is sw
    if (RtlCompareUnicodeString(&FileNameInfo->Name, &strSw, TRUE) == 0)
    {
      // concat
      NT_IF_FAIL_LEAVE(IMConcatStrings(&replacement, &FileNameInfo->ParentDir, &strHw));

      // then need to change
      NT_IF_FAIL_LEAVE(IoReplaceFileObjectName(FileObject, replacement.Buffer, replacement.Length));

      videoMode = IM_VIDEO_SW_TO_HW;
    }

    // by default it is no applicable
  }
  __finally
  {
    if (NULL != replacement.Buffer)
    {
      ExFreePool(replacement.Buffer);
    }

    if (NT_SUCCESS(status))
    {
      *VideoMode = videoMode;
    }
    else
    {
      *VideoMode = IM_VIDEO_ERROR;
    }
  }

  return status;
}

_Check_return_
    NTSTATUS
    IMDecideBlock(
        _In_ PUNICODE_STRING FullProcessName,
        _In_ PIM_NAME_INFORMATION FileNameInfo,
        _Out_ PBOOLEAN IsBlocked)
{
  UNICODE_STRING strAllowedExt = CONSTANT_STRING(IM_ALLOWED_EXTENTION);
  UNICODE_STRING strResticted = CONSTANT_STRING(IM_RESTRICTED_FILE);
  UNICODE_STRING strAllowedDir1 = CONSTANT_STRING(IM_ALLOWED_DIR_1);
  BOOLEAN isBlocked = FALSE;

  PAGED_CODE();

  __try
  {
    // we are only allow .dll files
    if (RtlCompareUnicodeString(&FileNameInfo->Extension, &strAllowedExt, TRUE) != 0)
    {
      isBlocked = TRUE;
      LOG(("[IM] Extention not a %wZ but %wZ\n", &strAllowedExt, &FileNameInfo->Extension));
      __leave;
    }

    // we restrict certain .dll files by checking is path contains
    if (IMIsContainsString(&FileNameInfo->FullName, &strResticted))
    {
      isBlocked = TRUE;
      LOG(("[IM] Restricted dll, %wZ\n", &strResticted));
      __leave;
    }

    // we only accept windows root folder and target process root folder
    if (!IMIsStartWithString(&FileNameInfo->ParentDir, &strAllowedDir1) && !IMIsStartWithString(FullProcessName, &FileNameInfo->ParentDir))
    {
      isBlocked = TRUE;
      LOG(("[IM] Allowed paths %wZ and %wZ, but we have %wZ\n", &strAllowedDir1, FullProcessName, &FileNameInfo->ParentDir));
      __leave;
    }
  }
  __finally
  {
    *IsBlocked = isBlocked;
  }

  return STATUS_SUCCESS;
}