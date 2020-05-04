/*++

author:

Daulet Tumbayev

Module Name:

InjectorMonitorKrnl.h

Abstract:
Interface between driver and user lib

Environment:

Kernel mode & User mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Definitions.
//------------------------------------------------------------------------

#define IM_PORT_NAME L"\\IMPort"

#define IM_AMOUNT_OF_DATA 2
#define IM_PROCESS_NAME_INDEX 0
#define IM_FILE_NAME_INDEX 1

//------------------------------------------------------------------------
//  Structures.
//------------------------------------------------------------------------

//
// Data of variable length in record
//
typedef struct _IM_KRECORD_DATA
{
  ULONG Size; // zero size in case if data was not retrieved
  PVOID Buffer;
} IM_KRECORD_DATA, *PIM_KRECORD_DATA; 

//
//  What information we actually log.
//
typedef struct _IM_KRECORD
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
  // contains data of variable length (such as strings)
  //
  IM_KRECORD_DATA Data[IM_AMOUNT_OF_DATA];

} IM_KRECORD, *PIM_KRECORD;

//
//  How the mini-filter manages the log records.
//
typedef struct _IM_KRECORD_LIST
{
  //
  // List element
  //
  LIST_ENTRY List;

  //
  // Data itself
  //
  IM_KRECORD Record;

} IM_KRECORD_LIST, *PIM_KRECORD_LIST;

//
//  Defines the commands between the utility and the filter
//
typedef enum _IM_INTERFACE_COMMAND
{
  NothingCommand = 0,
  //  fist 10 values are dedicated to driver working mode
  GetRecordsCommand = 11

} IM_INTERFACE_COMMAND;

#pragma warning(push)
#pragma warning(disable : 4200) // disable warnings for structures with zero length arrays.

//
// Communication structure
//
typedef struct _IM_COMMAND_MESSAGE
{
  IM_INTERFACE_COMMAND Command;
  ULONG Reserved;
  UCHAR Data[];

} IM_COMMAND_MESSAGE, *PIM_COMMAND_MESSAGE;

#pragma warning(pop)