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
#include "im_macro.h"

//------------------------------------------------------------------------
//  Definitions.
//------------------------------------------------------------------------

// todo consider to not to hardcode it.
#define IM_AMOUNT_OF_TARGET_PROCESSES 2
#define IM_HL_PROCESS_INFO_INDEX 0
#define IM_CS_PROCESS_INFO_INDEX 1
#define IM_HL_PROCESS_NAME L"hl.exe"
#define IM_CS_PROCESS_NAME L"csgo.exe"

//
// tags for memory
//
#define IM_KLIST_TAG ('IMkt')
#define IM_BUFFER_TAG ('IMbt')

//------------------------------------------------------------------------
//  Callback definitions.
//------------------------------------------------------------------------

typedef VOID (*IM_KELEMENT_FREE_CALLBACK)(PLIST_ENTRY ListEntry);

//------------------------------------------------------------------------
//  Structures.
//------------------------------------------------------------------------

//
// Similar to file name information
//
typedef struct _IM_NAME_INFORMATION
{
  // ParentDir + Name
  UNICODE_STRING FullName;

  // only name with extension without prefix backslash
  UNICODE_STRING Name;

  // Extension
  UNICODE_STRING Extension;

  // parent dir name from root to backslash
  UNICODE_STRING ParentDir;

} IM_NAME_INFORMATION, *PIM_NAME_INFORMATION;

//
// Information about our process
//
typedef struct _IM_PROCESS_INFO
{
  //
  // unique id from windows
  //
  HANDLE ProcessId;

  //
  // information about parent dir, name and extention
  //
  PIM_NAME_INFORMATION NameInfo;

  //
  // Name for which we are looking for
  //
  UNICODE_STRING TargetName;

  //
  // Process is now active
  //
  BOOLEAN isActive;

  //
  // Currently we are not allow duplications
  //
  BOOLEAN isDuplicate;

} IM_PROCESS_INFO, *PIM_PROCESS_INFO;

//
// List head in globals
//
typedef struct _IM_KLIST_HEAD
{
  //
  // size of IM_K*** struct
  //
  ULONG ElementStructSize;

  //
  //  Lookaside list used for allocating elements
  //
  NPAGED_LOOKASIDE_LIST ElementsLookaside;

  //
  //  Variable for maintaining sequence numbers.
  //
  __volatile LONGLONG SequenceNumber;

  //
  //  List of elements with data to send to user mode.
  //
  LIST_ENTRY ElementList;

  //
  //  Protection for the list of elements
  //
  KSPIN_LOCK ElementListLock;

  //
  // pushing element event
  //
  PKEVENT NewElementEvent;

  //
  //  Maximum amount of elements we could keep in memory
  //
  LONG MaxElementsToPush;

  //
  //  Current amount of elements we keep in memory
  //
  __volatile LONGLONG ElementsPushed;

  //
  // Callback to free list element
  //
  IM_KELEMENT_FREE_CALLBACK ElementFreeCallback;

} IM_KLIST_HEAD, *PIM_KLIST_HEAD;

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
  // logged records
  //
  IM_KLIST_HEAD RecordsHead;

  //
  // hl and cs processes info
  //
  IM_PROCESS_INFO TargetProcessInfo[IM_AMOUNT_OF_TARGET_PROCESSES];

} IM_GLOBALS, *PIM_GLOBALS;

extern IM_GLOBALS Globals; //  Global object itself