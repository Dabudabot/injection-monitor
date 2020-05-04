/*++

author:

Daulet Tumbayev

Module Name:

im.h

Abstract:
Globals for driver is here

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------

#include "fltKernel.h"
#include "InjectorMonitorKrnl.h"
#include "im_rec.h"
#include "im_macro.h"

//------------------------------------------------------------------------
//  Definitions.
//------------------------------------------------------------------------

#define IM_KRECORDS_TAG ('IMkt')
#define IM_BUFFER_TAG ('IMbt')

//------------------------------------------------------------------------
//  Structures.
//------------------------------------------------------------------------

//
// Global driver data structure
//
typedef struct _IM_GLOBALS
{

  //
  //  The object that identifies this driver.
  //
  PDRIVER_OBJECT DriverObject;

  //
  //  The filter that results from a call to
  //  FltRegisterFilter.
  //
  PFLT_FILTER Filter;

  //
  //  Server port: user mode connects to this port
  //

  PFLT_PORT ServerPort;

  //
  //  Client connection port: only one connection is allowed at a time.,
  //

  PFLT_PORT ClientPort;

  //
  //  Lookaside list used for allocating records
  //
  NPAGED_LOOKASIDE_LIST RecordsLookaside;

  //
  //  Variable for maintaining LogRecord sequence numbers.
  //
  __volatile LONGLONG LogSequenceNumber;

  //
  // blocked records
  //
  IM_KRECORD_HEAD RecordHead;

} IM_GLOBALS, *PIM_GLOBALS;

extern IM_GLOBALS Globals; //  Global object itself