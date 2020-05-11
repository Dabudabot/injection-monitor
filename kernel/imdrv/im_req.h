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
// Structures defenitions
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
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PIM_NAME_INFORMATION *NameInformation);

VOID IMReleaseNameInformation(
    _In_ PIM_NAME_INFORMATION NameInformation);