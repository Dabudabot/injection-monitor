/*++

author:

Daulet Tumbayev

Module Name:

im_list.h

Abstract:

List implementation

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im.h"

//------------------------------------------------------------------------
//  Function prototypes.
//------------------------------------------------------------------------

//
// List manipulations
//

_Check_return_
    NTSTATUS
    IMInitList(
        _Inout_ PIM_KLIST_HEAD ListHead,
        _In_ SIZE_T Size,
        _In_ LONG MaxElementsToPush,
        _In_ IM_KELEMENT_FREE_CALLBACK ElementFreeCallback);

VOID IMDeinitList(
    _Inout_ PIM_KLIST_HEAD ListHead);

VOID IMFreeList(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _In_ IM_KELEMENT_FREE_CALLBACK ElementFreeCallback);

VOID IMPush(
    _In_ PLIST_ENTRY ListEntry,
    _In_ PIM_KLIST_HEAD ListHead);

VOID IMPushToList(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY ListEntry,
    _In_opt_ PKEVENT Event);

VOID IMPop(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _Outptr_ PLIST_ENTRY *ListEntry);
