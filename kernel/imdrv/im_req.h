/*++

author:

Daulet Tumbayev

Module Name:

im_req.h

Abstract:
Header file which contains the function prototypes
of request

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------

#include "im.h"

//------------------------------------------------------------------------
//  Function prototypes
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMGetFileNameInformation(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation);

_Check_return_
    _IRQL_requires_(PASSIVE_LEVEL)
        NTSTATUS
    IMGetProcessNameInformation(
        _In_ HANDLE ProcessId,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation);

VOID IMReleaseNameInformation(
    _In_ PIM_NAME_INFORMATION NameInformation);