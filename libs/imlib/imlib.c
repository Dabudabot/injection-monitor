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
      IMDeinitilizeImpl(Context);
    }
  }

  return hResult;
}

VOID IMDeinitilizeImpl(
    _In_ PIM_CONTEXT Context)
{
  IF_FALSE_RETURN(Context != NULL);

  IMDeinitContext(Context);
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

  Context->Port = INVALID_HANDLE_VALUE;
  Context->RecordCallback = Callback;
  Context->Semaphore = INVALID_HANDLE_VALUE;
  Context->Thread = INVALID_HANDLE_VALUE;

  return S_OK;
}

VOID IMDeinitContext(
    _In_ PIM_CONTEXT Context)
{
  IF_FALSE_RETURN_RESULT(Context != NULL, E_INVALIDARG);

  if (INVALID_HANDLE_VALUE != Context->Port)
  {
    CloseHandle(Context->Port);
    Context->Port = INVALID_HANDLE_VALUE;
  }

  if (INVALID_HANDLE_VALUE != Context->Semaphore) // todo not sure
  {
    CloseHandle(Context->Semaphore);
    Context->Semaphore = INVALID_HANDLE_VALUE;
  }

  if (INVALID_HANDLE_VALUE != Context->Thread) // todo not sure
  {
    CloseHandle(Context->Thread);
    Context->Thread = INVALID_HANDLE_VALUE;
  }
}

_Check_return_
    HRESULT
    IMInitCollector(
        _In_ IM_RECORD_CALLBACK Callback,
        _In_ PIM_CONTEXT Context)
{
  // TODO
}