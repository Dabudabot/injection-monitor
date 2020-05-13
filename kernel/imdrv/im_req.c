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

#include "im_req.h"
#include "im_utils.h"

//------------------------------------------------------------------------
//  Local functions.
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMSplitNameInformation(
        _In_ PUNICODE_STRING FullName,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation);

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMGetFileNameInformation)
#pragma alloc_text(PAGE, IMGetProcessNameInformation)
#pragma alloc_text(PAGE, IMReleaseNameInformation)
#pragma alloc_text(PAGE, IMSplitNameInformation)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
// Undocumented funstions not found in the headers
//------------------------------------------------------------------------

NTSTATUS ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMGetFileNameInformation(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation)
/*++

Summary:

    This function gets the name information used to open the current file.

Arguments:

    Data            - A pointer to the callback data structure for the I/O operation.

    NameInformation - A pointer to a caller-allocated variable that receives the
                      address of a system-allocated FLT_FILE_NAME_INFORMATION structure
                      containing the file name information.

Return value:

    The return value is the status of the operation.

    ---- DO NOT FORGET TO FREE FILE NAME INFROMATION ------

--*/
{
  NTSTATUS status = STATUS_SUCCESS;
  PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Data != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(NameInformation != NULL, STATUS_INVALID_PARAMETER_2);

  *NameInformation = NULL;

  LOG(("[IM] Getting file name information\n"));

  __try
  {
    if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
    {
      // The SL_OPEN_TARGET_DIRECTORY flag indicates the caller is attempting
      // to open the target of a rename or hard link creation operation. We
      // must clear this flag when asking fltmgr for the name or the result
      // will not include the final component.
      ClearFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);

      // Get the filename as it appears below this filter. Note that we use
      // FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY when querying the filename
      // so that the filename as it appears below this filter does not end up
      // in filter manager's name cache.
      status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &fileNameInfo);

      // Restore the SL_OPEN_TARGET_DIRECTORY flag so the create will proceed
      // for the target. The file systems depend on this flag being set in
      // the target create in order for the subsequent SET_INFORMATION
      // operation to proceed correctly.
      SetFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);
    }
    else
    {
      // In some cases it is not safe for filter manager to generate a
      // file name, and FLT_FILE_NAME_QUERY_DEFAULT will detect those cases
      // and fail without looking in the cache.
      // FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP always checks the cache,
      // and then queries the file system if its safe.
      status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &fileNameInfo);
    }

    NT_IF_FAIL_LEAVE(status);

    LOG(("[IM] file name information: %wZ\n", &fileNameInfo->Name));

    NT_IF_FAIL_LEAVE(IMSplitNameInformation(&fileNameInfo->Name, NameInformation));
  }
  __finally
  {
    if (NULL != fileNameInfo)
    {
      FltReleaseFileNameInformation(fileNameInfo);
      fileNameInfo = NULL;
    }

    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Get file name information failed\n"));
      IMReleaseNameInformation(*NameInformation);
      (*NameInformation) = NULL;
    }
    else
    {
      LOG(("[IM] Got file with name %wZ\n", &(*NameInformation)->FullName));
    }
  }

  return status;
}

_Check_return_
    _IRQL_requires_(PASSIVE_LEVEL)
        NTSTATUS
    IMGetProcessNameInformation(
        _In_ HANDLE ProcessId,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation)
{
  NTSTATUS status = STATUS_SUCCESS;
  ULONG returnedLength = 0;
  HANDLE hProcess = NULL;
  PVOID buffer = NULL;
  PEPROCESS eProcess = NULL;

  PAGED_CODE();

  *NameInformation = NULL;

  LOG(("[IM] Getting process name information\n"));

  __try
  {
    NT_IF_FAIL_LEAVE(PsLookupProcessByProcessId(ProcessId, &eProcess));

    NT_IF_FAIL_LEAVE(ObOpenObjectByPointer(eProcess, 0, NULL, 0, 0, KernelMode, &hProcess));

    status = ZwQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       NULL, // buffer
                                       0,    // buffer size
                                       &returnedLength);

    if (STATUS_INFO_LENGTH_MISMATCH != status)
    {
      status = STATUS_UNSUCCESSFUL;
      LOG_B(("[IM] ZwQueryInformationProcess to get buffer size are failed\n"));
      __leave;
    }

    NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&buffer, returnedLength));

    NT_IF_FAIL_LEAVE(ZwQueryInformationProcess(hProcess,
                                               ProcessImageFileName,
                                               buffer,
                                               returnedLength,
                                               &returnedLength));

    NT_IF_FAIL_LEAVE(IMSplitNameInformation((PUNICODE_STRING)buffer, NameInformation));
  }
  __finally
  {
    if (NULL != buffer)
    {
      ExFreePool(buffer);
    }

    if (NULL != hProcess)
    {
      ZwClose(hProcess);
    }

    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Get process name information failed\n"));
      IMReleaseNameInformation(*NameInformation);
      (*NameInformation) = NULL;
    }
    else
    {
      LOG(("[IM] Got process with name %wZ\n", &(*NameInformation)->FullName));
    }
  }

  return status;
}

VOID IMReleaseNameInformation(
    _In_ PIM_NAME_INFORMATION NameInformation)
{
  PAGED_CODE();

  IF_FALSE_RETURN(NameInformation != NULL);

  if (NULL != NameInformation->ParentDir.Buffer)
  {
    ExFreePool(NameInformation->ParentDir.Buffer);
  }
  if (NULL != NameInformation->Name.Buffer)
  {
    ExFreePool(NameInformation->Name.Buffer);
  }
  if (NULL != NameInformation->FullName.Buffer)
  {
    ExFreePool(NameInformation->FullName.Buffer);
  }
  if (NULL != NameInformation->Extension.Buffer)
  {
    ExFreePool(NameInformation->Extension.Buffer);
  }

  ExFreePool(NameInformation);

  LOG(("[IM] Name information released\n"));
}

_Check_return_
    NTSTATUS
    IMSplitNameInformation(
        _In_ PUNICODE_STRING FullName,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation)
{
  NTSTATUS status = STATUS_SUCCESS;
  PIM_NAME_INFORMATION nameInfo = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(FullName != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(FullName->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(FullName->Length != 0, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(NameInformation != NULL, STATUS_INVALID_PARAMETER_2);

  *NameInformation = NULL;

  LOG(("[IM] Splitting name information\n"));

  __try
  {
    NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&nameInfo, sizeof(IM_NAME_INFORMATION)));

    NT_IF_FAIL_LEAVE(IMCopyUnicodeString(&nameInfo->FullName, FullName));

    NT_IF_FAIL_LEAVE(IMSplitString(&nameInfo->FullName, &nameInfo->ParentDir, &nameInfo->Name, L'\\', -1));

    NT_IF_FAIL_LEAVE(IMSplitString(&nameInfo->Name, NULL, &nameInfo->Extension, L'.', -1));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Split name information failed\n"));
      IMReleaseNameInformation(nameInfo);
    }
    else
    {
      LOG(("[IM] Name splitted. ParentDir: %wZ, Name: %wZ, Ext: %wZ\n", &nameInfo->ParentDir, &nameInfo->Name, &nameInfo->Extension));
      *NameInformation = nameInfo;
    }
  }

  return status;
}