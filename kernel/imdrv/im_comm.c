/*++

author:

Daulet Tumbayev

Module Name:

im_comm.h

Abstract:

Communication with user space

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im_comm.h"

//------------------------------------------------------------------------
//  Local functions definitions.
//------------------------------------------------------------------------

NTSTATUS
IMConnect(
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie);

VOID IMDisconnect(
    _In_opt_ PVOID ConnectionCookie);

NTSTATUS
IMMessage(
    _In_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength);

LONG IMExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer);

//
// Message functions
//

_Check_return_
    NTSTATUS
    IMGetRecords(
        _In_ PIM_KRECORD_HEAD RecordHead,
        _Out_ PVOID OutputBuffer,
        _In_ ULONG OutputBufferSize,
        _Out_ PULONG ReturnOutputBufferLength);

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMInitCommunication)
#pragma alloc_text(PAGE, IMDeinitCommunication)

// Local functions.
#pragma alloc_text(PAGE, IMConnect)
#pragma alloc_text(PAGE, IMDisconnect)
#pragma alloc_text(PAGE, IMMessage)
#pragma alloc_text(PAGE, IMExceptionFilter)

// Command handlers.
#pragma alloc_text(PAGE, IMGetRecords)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMInitCommunication(
        _In_ PFLT_FILTER Filter,
        _Outptr_ PFLT_PORT *Port)
{
  NTSTATUS status = STATUS_SUCCESS;
  PSECURITY_DESCRIPTOR sd = NULL;
  OBJECT_ATTRIBUTES oa;
  UNICODE_STRING uniString;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Filter != NULL, STATUS_INVALID_PARAMETER_1);

  LOG(("[IM] Communication initializing\n"));

  __try
  {
    NT_IF_FAIL_LEAVE(FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS));

    RtlInitUnicodeString(&uniString, IM_PORT_NAME);

    InitializeObjectAttributes(&oa,
                               &uniString,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               sd);

    NT_IF_FAIL_LEAVE(FltCreateCommunicationPort(Filter,
                                                Port,
                                                &oa,
                                                NULL,
                                                IMConnect,
                                                IMDisconnect,
                                                IMMessage,
                                                1));
  }
  __finally
  {
    if (NULL != sd)
    {
      FltFreeSecurityDescriptor(sd);
    }

    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Communication initializing failed\n"));
      IMDeinitCommunication(*Port);
    }
    else
    {
      LOG(("[IM] Communication initialized\n"));
    }
  }

  return status;
}

VOID IMDeinitCommunication(
    _In_ PFLT_PORT Port)
{
  PAGED_CODE();

  IF_FALSE_RETURN(Port != NULL);

  FltCloseCommunicationPort(Port);

  LOG(("[IM] Communication deinitialized\n"));
}

NTSTATUS
IMConnect(
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie)
{
  PAGED_CODE();

  UNREFERENCED_PARAMETER(ServerPortCookie);
  UNREFERENCED_PARAMETER(ConnectionContext);
  UNREFERENCED_PARAMETER(SizeOfContext);
  UNREFERENCED_PARAMETER(ConnectionCookie);

  FLT_ASSERT(Globals.ClientPort == NULL);
  Globals.ClientPort = ClientPort;

  LOG(("[IM] Client connected\n"));

  return STATUS_SUCCESS;
}

VOID IMDisconnect(
    _In_opt_ PVOID ConnectionCookie)
{
  PAGED_CODE();

  UNREFERENCED_PARAMETER(ConnectionCookie);

  FLT_ASSERT(Globals.Filter != NULL);
  FLT_ASSERT(Globals.ClientPort != NULL);

  //
  //  Close our handle
  //

  FltCloseClientPort(Globals.Filter, &Globals.ClientPort);

  LOG(("[IM] Client disconnected\n"));
}

NTSTATUS
IMMessage(
    _In_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength)
{
  IM_INTERFACE_COMMAND command;
  NTSTATUS status;

  PAGED_CODE();

  UNREFERENCED_PARAMETER(ConnectionCookie);

  //
  //                      **** PLEASE READ ****
  //
  //  The INPUT and OUTPUT buffers are raw user mode addresses.  The filter
  //  manager has already done a ProbedForRead (on InputBuffer) and
  //  ProbedForWrite (on OutputBuffer) which guarentees they are valid
  //  addresses based on the access (user mode vs. kernel mode).  The
  //  minifilter does not need to do their own probe.
  //
  //  The filter manager is NOT doing any alignment checking on the pointers.
  //  The minifilter must do this themselves if they care (see below).
  //
  //  The minifilter MUST continue to use a try/except around any access to
  //  these buffers.
  //

  if ((InputBuffer != NULL) &&
      (InputBufferSize >= (FIELD_OFFSET(IM_COMMAND_MESSAGE, Command) +
                           sizeof(IM_INTERFACE_COMMAND))))
  {

    __try
    {

      //
      //  Probe and capture input message: the message is raw user mode
      //  buffer, so need to protect with exception handler
      //

      command = ((PIM_COMMAND_MESSAGE)InputBuffer)->Command;
    }
    __except (IMExceptionFilter(GetExceptionInformation(), TRUE))
    {

      return GetExceptionCode();
    }

    LOG(("[IM] Got new message with command 0x%x\n", command));

    if (command == GetRecordsCommand)
    {
      //
      //  Return as many log records as can fit into the OutputBuffer
      //

      if ((OutputBuffer == NULL) || (OutputBufferSize == 0))
      {

        status = STATUS_INVALID_PARAMETER;
        LOG_B(("[IM] message processed with status invalid parameter\n"));
        return status;
      }

      //
      //  We want to validate that the given buffer is POINTER
      //  aligned.  But if this is a 64bit system and we want to
      //  support 32bit applications we need to be careful with how
      //  we do the check.  Note that the way SpyGetLog is written
      //  it actually does not care about alignment but we are
      //  demonstrating how to do this type of check.
      //

#if defined(_WIN64)

      if (IoIs32bitProcess(NULL))
      {

        //
        //  Validate alignment for the 32bit process on a 64bit
        //  system
        //

        if (!IS_ALIGNED(OutputBuffer, sizeof(ULONG)))
        {

          status = STATUS_DATATYPE_MISALIGNMENT;
          LOG_B(("[IM] message processed with STATUS_DATATYPE_MISALIGNMENT\n"));
          return status;
        }
      }
      else
      {

#endif

        if (!IS_ALIGNED(OutputBuffer, sizeof(PVOID)))
        {

          status = STATUS_DATATYPE_MISALIGNMENT;
          LOG_B(("[IM] message processed with STATUS_DATATYPE_MISALIGNMENT\n"));
          return status;
        }

#if defined(_WIN64)
      }

#endif

      //
      //  Get the log record.
      //

      status = IMGetRecords(
          &Globals.RecordHead,
          OutputBuffer,
          OutputBufferSize,
          ReturnOutputBufferLength);
    }
    else
    {
      status = STATUS_INVALID_PARAMETER;
      LOG_B(("[IM] message processed with STATUS_INVALID_PARAMETER\n"));
      return status;
    }
  }
  else
  {
    status = STATUS_INVALID_PARAMETER;
    LOG_B(("[IM] message processed with STATUS_INVALID_PARAMETER\n"));
  }

  LOG(("[IM] Message processed with status 0x%x", status));
  return status;
}

LONG IMExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer)
{
  NTSTATUS Status;

  PAGED_CODE();

  LOG_B(("[IM] Exception\n"));

  Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

  //
  //  Certain exceptions shouldn't be dismissed within the namechanger filter
  //  unless we're touching user memory.
  //

  if (!FsRtlIsNtstatusExpected(Status) &&
      !AccessingUserBuffer)
  {

    return EXCEPTION_CONTINUE_SEARCH;
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

//
// Message functions
//

_Check_return_
    NTSTATUS
    IMGetRecords(
        _In_ PIM_KRECORD_HEAD RecordHead,
        _Out_ PVOID OutputBuffer,
        _In_ ULONG OutputBufferSize,
        _Out_ PULONG ReturnOutputBufferLength)
{
  NTSTATUS status = STATUS_SUCCESS;
  PLIST_ENTRY currentEntry;
  KIRQL oldIrql;
  PCHAR buffer = OutputBuffer;
  PIM_KRECORD_LIST recordList;
  ULONG copiedLen = 0;

  PKSPIN_LOCK listLock = &RecordHead->RecordListLock;
  PLIST_ENTRY listEntry = &RecordHead->RecordList;
  ULONG sizeOfRecord = RecordHead->RecordStructSize;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(RecordHead != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(OutputBuffer != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(OutputBufferSize != 0, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(ReturnOutputBufferLength != NULL, STATUS_INVALID_PARAMETER_4);

  LOG(("[IM] Records copy start\n"));

  KeAcquireSpinLock(listLock, &oldIrql);

  while (!IsListEmpty(listEntry))
  {
    currentEntry = RemoveHeadList(listEntry);

    KeReleaseSpinLock(listLock, oldIrql);

    recordList = CONTAINING_RECORD(currentEntry, IM_KRECORD_LIST, List);

    if ((ULONG)OutputBufferSize < copiedLen + recordList->Record.TotalLength)
    {
      //need to put last record back and break because of overflow
      KeAcquireSpinLock(listLock, &oldIrql);
      InsertHeadList(listEntry, currentEntry);
      break;
    }

    copiedLen += recordList->Record.TotalLength;

    // extract record itself and copy to buffer

    __try
    {
      RtlCopyMemory(buffer, &recordList->Record, sizeOfRecord);
      buffer += sizeOfRecord;

      for (ULONG i = 0; i < IM_AMOUNT_OF_DATA; i++)
      {
        if (recordList->Record.Data[i].Size == 0)
          continue;
        RtlCopyMemory(buffer, recordList->Record.Data[i].Buffer, recordList->Record.Data[i].Size);
        buffer += recordList->Record.Data[i].Size;
      }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      KeAcquireSpinLock(listLock, &oldIrql);
      InsertHeadList(listEntry, currentEntry);
      KeReleaseSpinLock(listLock, oldIrql);
      ASSERT(FALSE);
      return GetExceptionCode();
    }

    IMFreeRecord(recordList);

    InterlockedDecrement64(&RecordHead->RecordsPushed);
    ASSERT(RecordHead->RecordsAllocated >= 0); // todo sometimes it decs earlier than incs

    KeAcquireSpinLock(listLock, &oldIrql);
  }

  KeReleaseSpinLock(listLock, oldIrql);

  // if at least one record was copied, return success
  if (copiedLen > 0)
  {
    LOG(("[IM] Copied bytes to user space = %d\n", copiedLen));

    *ReturnOutputBufferLength = copiedLen;

    return STATUS_SUCCESS;
  }

  LOG(("[IM] No records were copied\n"));

  return STATUS_NO_MORE_ENTRIES;
}