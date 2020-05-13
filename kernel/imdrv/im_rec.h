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
//  Function prototypes.
//------------------------------------------------------------------------

_Check_return_
    _IRQL_requires_max_(APC_LEVEL)
        NTSTATUS
    IMCreateRecord(
        _Outptr_ PIM_KRECORD_LIST *RecordList,
        _In_ PFLT_CALLBACK_DATA Data,
        _In_ PIM_NAME_INFORMATION FileNameInformation,
        _In_ PUNICODE_STRING ProcessName,
        _In_ IM_VIDEO_MODE_STATUS VideoMode);

VOID IMFreeRecord(
    _In_ PIM_KRECORD_LIST RecordList);

VOID IMFreeRecordList(
    _In_ PLIST_ENTRY ListEntry);
