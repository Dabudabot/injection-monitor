/*++

author:

Daulet Tumbayev

Module Name:

imlib.c

Abstract:
implementation of library

Environment:

User mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "InjectorMonitor.h"
#include "imlib_macro.h"
#include "InjectorMonitorKrnl.h"
#include "fltUser.h"
#include "stdlib.h"

//------------------------------------------------------------------------
//  Definitions.
//------------------------------------------------------------------------

#define BUFFER_SIZE 4096

//------------------------------------------------------------------------
//  Local globals.
//------------------------------------------------------------------------

//
// Globals of the lib
//
typedef struct _IM_CONTEXT
{
  //
  // Thread which request driver
  //
  HANDLE Thread;

  //
  // port to driver
  //
  HANDLE Port;

  //
  // sync for thread
  //
  HANDLE Semaphore;

  //
  //
  //
  BOOLEAN isDown;

  //
  // callback from user app
  //
  IM_RECORD_CALLBACK RecordCallback;

} IM_CONTEXT, *PIM_CONTEXT;

IM_CONTEXT Globals;

//------------------------------------------------------------------------
//  Local functions defintions.
//------------------------------------------------------------------------

_Check_return_
    HRESULT
    IMInitilizeImpl(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context);

VOID IMDeinitilizeImpl(
    _In_ PIM_CONTEXT Context);

_Check_return_
    HRESULT
    IMConnect(
        _In_ LPCWSTR PortName,
        _Outptr_ HANDLE *Port);

_Check_return_
    HRESULT
    IMInitContext(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context);

VOID IMDeinitContext(
    _In_ PIM_CONTEXT Context);

_Check_return_
    HRESULT
    IMInitCollector(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context);

DWORD
WINAPI
IMRetrieveRecords(
    _In_ LPVOID lpParameter);

_Check_return_
    HRESULT
    IMMoveRecord(
        PCHAR *SourceRecord,
        PIM_RECORD *TargetRecord,
        PULONG Pointer);

_Check_return_
HRESULT
IMSend(
HANDLE Port,
ULONG Command,
_Inout_ PCHAR Buffer,
_In_ ULONG BufferSize,
_Inout_ PULONG ReturnLen);

VOID IMFreeRecord(PIM_RECORD Record);

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

_Check_return_
    HRESULT
    IMInitilize(
        _In_ IM_RECORD_CALLBACK Callback)
{
  return IMInitilizeImpl(Callback, &Globals);
}

_Check_return_
    HRESULT
    IMDeinitilize()
{
  IMDeinitilizeImpl(&Globals);
  return S_OK;
}

//------------------------------------------------------------------------

_Check_return_
    HRESULT
    IMInitilizeImpl(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context)
{
  HRESULT hResult = S_OK;
  HANDLE port = INVALID_HANDLE_VALUE;

  IF_FALSE_RETURN_RESULT(Callback != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Context != NULL, E_INVALIDARG);

  LOG(("[IM] Library initialization\n"));

  __try
  {
    HR_IF_FAIL_LEAVE(IMInitContext(Callback, Context));

    HR_IF_FAIL_LEAVE(IMConnect(IM_PORT_NAME, &Context->Port));

    HR_IF_FAIL_LEAVE(IMInitCollector(Callback, Context));
  }
  __finally
  {
    if (FAILED(hResult))
    {
      LOG_B(("[IM] Library initialization failed\n"));
      IMDeinitilizeImpl(Context);
    }
    else
    {
      LOG(("[IM] Library initialized\n"));
    }
  }

  return hResult;
}

VOID IMDeinitilizeImpl(
    _In_ PIM_CONTEXT Context)
{
  IF_FALSE_RETURN(Context != NULL);

  IMDeinitContext(Context);

  LOG(("[IM] Library deinitialized\n"));
}

_Check_return_
    HRESULT
    IMConnect(
        _In_ LPCWSTR PortName,
        _Outptr_ HANDLE *Port)
{
  HRESULT hResult = S_OK;

  IF_FALSE_RETURN_RESULT(PortName != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Port != NULL, E_INVALIDARG);

  LOG(("[IM] Connecting to the kernel component\n"));

  hResult = FilterConnectCommunicationPort(PortName,
                                           0,
                                           NULL,
                                           0,
                                           NULL,
                                           Port);

  if (E_ACCESSDENIED == hResult)
  {
    printf("[IM] You must run as administrator, failed to establish kernel connection\n");
  }

  if (FAILED(hResult))
  {
    LOG_B(("[IM] Kernel connection failed\n"));
  }
  else
  {
    LOG(("[IM] Kernel component connected\n"));
  }

  return hResult;
}

_Check_return_
    HRESULT
    IMInitContext(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context)
{
  IF_FALSE_RETURN_RESULT(Callback != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Context != NULL, E_INVALIDARG);

  LOG(("[IM] Library context initialized\n"));

  Context->Port = INVALID_HANDLE_VALUE;
  Context->RecordCallback = Callback;
  Context->Semaphore = INVALID_HANDLE_VALUE;
  Context->Thread = INVALID_HANDLE_VALUE;
  Context->isDown = FALSE;

  return S_OK;
}

VOID IMDeinitContext(
    _In_ PIM_CONTEXT Context)
{
  IF_FALSE_RETURN(Context != NULL);

  LOG(("[IM] Waiting to kill requester thread\n"));

  Context->isDown = TRUE;

  WaitForSingleObject(Context->Semaphore, INFINITE);

  if (INVALID_HANDLE_VALUE != Context->Port)
  {
    CloseHandle(Context->Port);
    Context->Port = INVALID_HANDLE_VALUE;
    LOG(("[IM] Port closed\n"));
  }

  if (INVALID_HANDLE_VALUE != Context->Semaphore) // todo not sure
  {
    CloseHandle(Context->Semaphore);
    Context->Semaphore = INVALID_HANDLE_VALUE;
    LOG(("[IM] Semaphore closed\n"));
  }

  if (INVALID_HANDLE_VALUE != Context->Thread) // todo not sure
  {
    CloseHandle(Context->Thread);
    Context->Thread = INVALID_HANDLE_VALUE;
    LOG(("[IM] Thread closed\n"));
  }

  LOG(("[IM] Library context deinitialized\n"));
}

_Check_return_
    HRESULT
    IMInitCollector(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context)
{
  ULONG threadId;
  HRESULT hResult = S_OK;

  IF_FALSE_RETURN_RESULT(Callback != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Context != NULL, E_INVALIDARG);

  LOG(("[IM] Requester thread initialization\n"));

  Context->Semaphore = CreateSemaphoreW(NULL,
                                        0,
                                        1,
                                        L"monitor shut down");

  if (NULL == Context->Semaphore)
  {
    hResult = GetLastError();
    LOG_B(("[im] error semaphore %d\n", hResult));
    return hResult;
  }

  Context->Thread = CreateThread(
      NULL,
      0,
      IMRetrieveRecords,
      (LPVOID)Context,
      0, &threadId);

  if (!Context->Thread)
  {
    hResult = GetLastError();
    LOG_B(("[IM] Could not create logging thread: %d\n", hResult));
    return hResult;
  }

  LOG(("[IM] Requester thread initialized\n"));

  return hResult;
}

DWORD
WINAPI
IMRetrieveRecords(
    _In_ LPVOID lpParameter)
{
  PIM_CONTEXT context = NULL;
  HRESULT hResult = S_OK;
  PVOID alignedBuffer[BUFFER_SIZE / sizeof(PVOID)];
  PCHAR buffer = (PCHAR)alignedBuffer;
  ULONG returnLen = 0;
  ULONG ttl = 10;
  PVOID recordsBufferPointer;
  ULONG i;
  PIM_RECORD record;

  IF_FALSE_RETURN_RESULT(lpParameter != NULL, E_INVALIDARG);

  context = (PIM_CONTEXT)lpParameter;

  LOG(("[IM] Requester thread loop entering: \n"));

#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant

  while (TRUE)
  {

#pragma warning(pop)
    i = 0;
    returnLen = 0;

    if (context->isDown)
    {
      break;
    }

    hResult = IMSend(
        context->Port,
        GetRecordsCommand,
        buffer,
        sizeof(alignedBuffer),
        &returnLen);

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hResult)
    {
      //LOG(("  [IM] No items from kernel\n"));
      Sleep(200);
      continue;
    }

    if (IS_ERROR(hResult))
    {
      if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hResult)
      {
        LOG(("[IM] The kernel component of minispy has unloaded. Exiting. Consider deinitialization\n"));
        break;
      }

      LOG_B(("[IM] error send GetRecordsCommand\n"));
      ttl--;
      if (ttl == 0)
      {
        LOG_B(("[IM] error send GetRecordsCommand too many errors\n"));
        break;
      }
      continue;
    }
    else
    {
      ttl = 10;
    }

    recordsBufferPointer = buffer;

    while (i < returnLen)
    {
      hResult = IMMoveRecord((PCHAR *)&recordsBufferPointer, &record, &i);

      if (IS_ERROR(hResult))
      {
        LOG_B(("[IM] error move record\n"));
        continue;
      }

      LOG(("  [IM] Sending item to callback\n"));
      context->RecordCallback(record);

      IMFreeRecord(record);
    }
  }

  LOG(("[IM] Requestor loop broken\n"));

  ReleaseSemaphore(context->Semaphore, 1, NULL);

  return 0;
}

HRESULT
IMSend(
    HANDLE Port,
    ULONG Command,
    IN OUT PCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG ReturnLen)
{
  HRESULT hResult = S_OK;
  IM_COMMAND_MESSAGE command;

  IF_FALSE_RETURN_RESULT(Port != INVALID_HANDLE_VALUE, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Buffer != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Buffer != 0, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(ReturnLen != NULL, E_INVALIDARG);

  //LOG(("[IM] Sending message to kernel component 0x%x\n", Command));

  command.Command = (IM_INTERFACE_COMMAND)Command;

  hResult = FilterSendMessage(
      Port,
      &command,
      sizeof(IM_COMMAND_MESSAGE),
      (LPVOID)Buffer,
      BufferSize,
      ReturnLen);

  //LOG(("[IM] Kernel respond 0x%x\n", hResult));

  return hResult;
}

_Check_return_
    HRESULT
    IMMoveRecord(
        PCHAR *SourceRecord,
        PIM_RECORD *TargetRecord,
        PULONG Pointer)
{
  PCHAR sourceRecord = NULL;
  PIM_KRECORD kernelRecord;
  PIM_RECORD record = NULL;
  ULONG i;
  PVOID tempBuffer;

  IF_FALSE_RETURN_RESULT(SourceRecord != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(TargetRecord != NULL, E_INVALIDARG);
  IF_FALSE_RETURN_RESULT(Pointer != NULL, E_INVALIDARG);

  sourceRecord = *SourceRecord;

  IF_FALSE_RETURN_RESULT(sourceRecord != NULL, E_INVALIDARG);

  LOG(("[IM] Moving kernel record to user record\n"));

  kernelRecord = (PIM_KRECORD)sourceRecord;
  record = (PIM_RECORD)malloc(sizeof(IM_RECORD));

  IF_FALSE_RETURN_RESULT(record != NULL, E_OUTOFMEMORY);

  ZeroMemory(record, sizeof(IM_RECORD));

  if (0xCEFAADDE != kernelRecord->Debug)
  {
    LOG_B(("[IM] struct offset is wrong\n"));
    return E_UNEXPECTED;
  }

  record->Debug = 0xCEFAADDE;
  record->TotalLength = sizeof(IM_RECORD) + kernelRecord->Data[IM_PROCESS_NAME_INDEX].Size + kernelRecord->Data[IM_FILE_NAME_INDEX].Size;
  record->SequenceNumber = kernelRecord->SequenceNumber;
  record->Time = kernelRecord->Time;
  record->IsBlocked = kernelRecord->IsBlocked;
  record->IsSucceded = kernelRecord->IsSucceded;

  record->ProcessNameLength = kernelRecord->Data[IM_PROCESS_NAME_INDEX].Size / sizeof(WCHAR);
  record->FileNameLenght = kernelRecord->Data[IM_FILE_NAME_INDEX].Size / sizeof(WCHAR);

  sourceRecord += sizeof(IM_KRECORD);
  *Pointer += sizeof(IM_KRECORD);

  for (i = 0; i < IM_AMOUNT_OF_DATA; i++)
  {
    if (kernelRecord->Data[i].Size == 0)
    {
      continue;
    }

    tempBuffer = malloc(kernelRecord->Data[i].Size);
    RtlCopyMemory(tempBuffer, sourceRecord, kernelRecord->Data[i].Size);

    switch (i)
    {
    case IM_PROCESS_NAME_INDEX:
      record->ProcessName = tempBuffer;
      break;
    case IM_FILE_NAME_INDEX:
      record->FileName = tempBuffer;
      break;
    default:
      LOG_B(("[IM] Unexpected index\n"));
      free(tempBuffer);
      break;
    }

    sourceRecord += kernelRecord->Data[i].Size;
    *Pointer += kernelRecord->Data[i].Size;
  }

  *SourceRecord = sourceRecord;
  *TargetRecord = record;

  LOG(("[IM] Record moved\n"));

  return S_OK;
}

VOID IMFreeRecord(PIM_RECORD Record)
{
  IF_FALSE_RETURN(Record != NULL);

  if (Record->FileNameLenght != 0 && Record->FileName != NULL)
  {
    free(Record->FileName);
  }

  if (Record->ProcessNameLength != 0 && Record->ProcessName != NULL)
  {
    free(Record->ProcessName);
  }

  free(Record);

  LOG(("[IM] Record freed\n"));
}