/*++

author:

Daulet Tumbayev

Module Name:

InjectorMonitor.h

Abstract:
Interface lib and user app

Environment:

User mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "Windows.h"

//------------------------------------------------------------------------
//  Macro definitions.
//------------------------------------------------------------------------

//
// in case of c++ in user app
//
#if defined(__cplusplus)
#define IM_API extern "C" HRESULT
#else
#define IM_API HRESULT
#endif

//------------------------------------------------------------------------
//  Structures.
//------------------------------------------------------------------------

//
// Record with all data of log from driver
//
typedef struct _IM_RECORD
{
  //
  // Just marker for the debug to navigate in memory
  // equals 0xCEFAADDE which will be in memory shown as FACEDEAD due to little indean
  //
  ULONG Debug;

  //
  // Length of the structure (it is variable because of string inside)
  //
  ULONG TotalLength;

  //
  // Order number of the record for logging
  //
  ULONGLONG SequenceNumber;

  //
  // Time when record was created, also for logging
  //
  LARGE_INTEGER Time;

  //
  // Is loading of the binary is blocked or not
  //
  BOOLEAN IsBlocked;

  //
  // is pre operation callback succeded (just for log)
  //
  BOOLEAN IsSucceded;

  //
  // amount of symbols in process name without '\0'
  //
  ULONG ProcessNameLength;

  //
  // process name (pointer allocated and removed in library)
  //
  PWCHAR ProcessName;

  //
  // amount of symbols in file name without '\0'
  //
  ULONG FileNameLenght;

  //
  // file name (pointer allocated and removed in library)
  //
  PWCHAR FileName;

} IM_RECORD, *PIM_RECORD;

//------------------------------------------------------------------------
//  Callbacks definitions.
//------------------------------------------------------------------------

//
// This callback will be triggered when driver will decide to send to user some record
//
typedef HRESULT (*IM_RECORD_CALLBACK)(PIM_RECORD Record);

//------------------------------------------------------------------------
//  Function defintions.
//------------------------------------------------------------------------

_Check_return_
    IM_API
    IMInitilize(
        _In_ IM_RECORD_CALLBACK Callback);

_Check_return_
    IM_API
    IMDeinitilize();