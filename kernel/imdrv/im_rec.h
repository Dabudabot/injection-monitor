/*++

author:

Daulet Tumbayev

Module Name:

im_rec.h

Abstract:

Record control function definition and structures

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im.h"

//------------------------------------------------------------------------
//  Defines.
//------------------------------------------------------------------------

#define IM_DEFAULT_MAX_RECORDS 100

//------------------------------------------------------------------------
//  Function prototypes.
//------------------------------------------------------------------------

//
// List manipulations
//

_Check_return_
    NTSTATUS
    IMInitList(
        _Inout_ PIM_KRECORD_HEAD RecordHead,
        _In_ SIZE_T Size);

VOID IMDeinitList(
    _Inout_ PIM_KRECORD_HEAD RecordHead);

VOID IMFreeList(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead);

VOID IMPush(
    _In_ PLIST_ENTRY ListEntry,
    _In_ PIM_KRECORD_HEAD RecordsHead);

VOID IMPushToList(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY ListEntry,
    _In_opt_ PKEVENT Event);

VOID IMPop(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _Outptr_ PLIST_ENTRY *RecordListEntry);

_Check_return_
    NTSTATUS
    IMCreateRecord(
        _Outptr_ PIM_KRECORD_LIST *RecordList,
        _In_ PFLT_CALLBACK_DATA Data,
        _In_ PUNICODE_STRING FileName,
        _In_ PUNICODE_STRING ProcessName);

VOID IMFreeRecord(
    _In_ PIM_KRECORD_LIST RecordList);
