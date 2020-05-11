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

//------------------------------------------------------------------------
//  Defines.
//------------------------------------------------------------------------

#define IM_TARGET_PROCESS_NAME_1 L"hl.exe"   // todo consider to not to hardcode it.
#define IM_TARGET_PROCESS_NAME_2 L"csgo.exe" // todo consider to not to hardcode it.

#define IM_ALLOWED_EXTENTION L"dll"

#define IM_RESTRICTED_FILE L"Steam\\crashhandler.dll" //consider to not to hardcode it

#define IM_ALLOWED_DIR_1 L"\\Device\\HarddiskVolume3\\Windows\\" // todo look for right device harddisk

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMPreCreate)
#pragma alloc_text(PAGE, IMPostCreate)
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

  *CompletionContext = NULL;

  PAGED_CODE();

  UNREFERENCED_PARAMETER(FltObjects);

  // We are only registered for the IRP_MJ_CREATE.
  FLT_ASSERT(Data != NULL);
  FLT_ASSERT(Data->Iopb != NULL);
  FLT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_CREATE);

  desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;

  if (FlagOn(desiredAccess, FILE_EXECUTE)) // CHECK THIS
  {
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
  }
  else
  {
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
  }
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

  PIM_NAME_INFORMATION processNameInfo = NULL;
  PIM_NAME_INFORMATION fileNameInfo = NULL;

  BOOLEAN isBlocked = FALSE;

  UNICODE_STRING strTargetProcess1 = CONSTANT_STRING(IM_TARGET_PROCESS_NAME_1);
  UNICODE_STRING strTargetProcess2 = CONSTANT_STRING(IM_TARGET_PROCESS_NAME_2);
  UNICODE_STRING strAllowedExt = CONSTANT_STRING(IM_ALLOWED_EXTENTION);
  UNICODE_STRING strResticted = CONSTANT_STRING(IM_RESTRICTED_FILE);
  UNICODE_STRING strAllowedDir1 = CONSTANT_STRING(IM_ALLOWED_DIR_1);

  PAGED_CODE();

  //DbgBreakPoint();

  UNREFERENCED_PARAMETER(Flags);
  UNREFERENCED_PARAMETER(CompletionContext);

  // We are only registered for the IRP_MJ_CREATE.
  FLT_ASSERT(Data != NULL);
  FLT_ASSERT(Data->Iopb != NULL);
  FLT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_CREATE);

  LOG(("[IM] Post create start\n"));

  __try
  {
    // first we have to get our process name information
    NT_IF_FAIL_LEAVE(IMGetProcessNameInformation(Data, &processNameInfo));

    // we are looking only for specific process names
    if (RtlCompareUnicodeString(&processNameInfo->Name, &strTargetProcess1, TRUE) != 0 && RtlCompareUnicodeString(&processNameInfo->Name, &strTargetProcess2, TRUE) != 0)
    {
      LOG(("[IM] This, %wZ, is not our process name, we are looking for %wZ or %wZ\n", &processNameInfo->Name, &strTargetProcess1, &strTargetProcess2));
      __leave;
    }

    // get file info of the file witch are opening by the process
    NT_IF_FAIL_LEAVE(IMGetFileNameInformation(Data, &fileNameInfo));

    // now we know that target process are trying to open for execution something
    // it is enouth evidence to log it. So record will be created
    NT_IF_FAIL_LEAVE(IMCreateRecord(&recordList, Data, &fileNameInfo->FullName, &processNameInfo->FullName));

    // we are only allow .dll files
    if (RtlCompareUnicodeString(&fileNameInfo->Extension, &strAllowedExt, TRUE) != 0)
    {
      isBlocked = TRUE;
      LOG(("[IM] Extention not a %wZ but %wZ\n", &strAllowedExt, &fileNameInfo->Extension));
      __leave;
    }

    // we restrict certain .dll files by checking is path contains
    if (IMIsContainsString(&fileNameInfo->FullName, &strResticted)) // CHECK THIS
    {
      isBlocked = TRUE;
      LOG(("[IM] Restricted dll, %wZ\n", &strResticted));
      __leave;
    }

    // we only accept windows root folder and target process root folder
    if (!IMIsStartWithString(&fileNameInfo->ParentDir, &strAllowedDir1) && RtlCompareUnicodeString(&fileNameInfo->ParentDir, &processNameInfo->ParentDir, TRUE) != 0) // CHECK THIS
    {
      isBlocked = TRUE;
      LOG(("[IM] Allowed paths %wZ and %wZ, but we have %wZ\n", &strAllowedDir1, &processNameInfo->ParentDir, &fileNameInfo->ParentDir));
      __leave;
    }
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Operatin failed\n"));
    }

    if (NULL != fileNameInfo)
    {
      IMReleaseNameInformation(fileNameInfo);
    }

    if (NULL != processNameInfo)
    {
      IMReleaseNameInformation(processNameInfo);
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
      IMPush(&recordList->List, &Globals.RecordHead);
    }
  }

  return FLT_POSTOP_FINISHED_PROCESSING;
}