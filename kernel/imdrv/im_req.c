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
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMGetFileNameInformation)
#pragma alloc_text(PAGE, IMGetProcessNameInformation)
#pragma alloc_text(PAGE, IMReleaseProcessNameInformation)
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
        _Outptr_ PFLT_FILE_NAME_INFORMATION *NameInformation)
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
  PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Data != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(NameInformation != NULL, STATUS_INVALID_PARAMETER_2);

  *NameInformation = NULL;

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
      status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &nameInfo);

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
      status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &nameInfo);
    }

    NT_IF_FAIL_LEAVE(status);

    *NameInformation = nameInfo;
  }
  __finally
  {
    if (!NT_SUCCESS(status))
    {
      if (nameInfo != NULL)
      {
        FltReleaseFileNameInformation(nameInfo);
        nameInfo = NULL;
      }
    }
  }

  return status;
}

_Check_return_
    _IRQL_requires_(PASSIVE_LEVEL)
        NTSTATUS
    IMGetProcessNameInformation(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PIM_PROCESS_NAME_INFORMATION *NameInformation)
{
  NTSTATUS status = STATUS_SUCCESS;
  ULONG returnedLength = 0;
  ULONG bufferLength = 0;
  HANDLE hProcess = NULL;
  PVOID buffer = NULL;
  PEPROCESS eProcess = NULL;
  PIM_PROCESS_NAME_INFORMATION nameInfo = NULL;

  PAGED_CODE();

  *NameInformation = NULL;

  __try
  {
    eProcess = PsGetCurrentProcess();

    if (NULL == eProcess)
    {
      __leave;
    }

    NT_IF_FAIL_LEAVE(ObOpenObjectByPointer(eProcess, 0, NULL, 0, 0, KernelMode, &hProcess));

    status = ZwQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       NULL, // buffer
                                       0,    // buffer size
                                       &returnedLength);

    if (STATUS_INFO_LENGTH_MISMATCH != status)
    {
      status = STATUS_UNSUCCESSFUL;
      __leave;
    }

    NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&buffer, returnedLength));

    NT_IF_FAIL_LEAVE(ZwQueryInformationProcess(hProcess,
                                               ProcessImageFileName,
                                               buffer,
                                               returnedLength,
                                               &returnedLength));

    NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&nameInfo, sizeof(IM_PROCESS_NAME_INFORMATION)));

    NT_IF_FAIL_LEAVE(IMCopyUnicodeString(&nameInfo->FullName, (PUNICODE_STRING)buffer));

    NT_IF_FAIL_LEAVE(IMSplitString(&nameInfo->FullName, &nameInfo->ParentDir, &nameInfo->ProcessName, L'\\', -1));
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
      IMReleaseProcessNameInformation(nameInfo);
    }
    else
    {
      *NameInformation = nameInfo;
    }
  }

  return status;
}

VOID IMReleaseProcessNameInformation(
    _In_ PIM_PROCESS_NAME_INFORMATION NameInformation)
{
  PAGED_CODE();

  IF_FALSE_RETURN(NameInformation != NULL);

  if (NULL != NameInformation->ParentDir.Buffer)
  {
    ExFreePool(NameInformation->ParentDir.Buffer);
  }
  if (NULL != NameInformation->ProcessName.Buffer)
  {
    ExFreePool(NameInformation->ProcessName.Buffer);
  }
  if (NULL != NameInformation->FullName.Buffer)
  {
    ExFreePool(NameInformation->FullName.Buffer);
  }

  ExFreePool(NameInformation);
}