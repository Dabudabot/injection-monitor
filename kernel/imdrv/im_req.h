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
typedef struct _IM_PROCESS_NAME_INFORMATION
{
  // ParentDir + ProcessName
  UNICODE_STRING FullName;

  // only process name without prefix backslash
  UNICODE_STRING ProcessName;

  // parent dir name from root to backslash
  UNICODE_STRING ParentDir;

} IM_PROCESS_NAME_INFORMATION, *PIM_PROCESS_NAME_INFORMATION;

//------------------------------------------------------------------------
//  Function prototypes
//------------------------------------------------------------------------

_Check_return_
    NTSTATUS
    IMGetFileNameInformation(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PFLT_FILE_NAME_INFORMATION *NameInformation);

_Check_return_
_IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    IMGetProcessNameInformation(
        _Inout_ PFLT_CALLBACK_DATA Data,
        _Outptr_ PIM_PROCESS_NAME_INFORMATION *NameInformation);

VOID IMReleaseProcessNameInformation(
    _In_ PIM_PROCESS_NAME_INFORMATION NameInformation);