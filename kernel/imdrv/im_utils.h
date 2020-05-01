/*++

author:

Daulet Tumbayev

Module Name:

im_utils.h

Abstract:

Allocaion, free, copy functions for buffers, resources, strings

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
// Resources
//

_Check_return_
    NTSTATUS
    IMAllocateResource(
        _Outptr_ PERESOURCE *Resource);

VOID IMFreeResource(
    _In_ PERESOURCE Resource);

//
// Buffers
//

_Check_return_
    NTSTATUS
    IMAllocateNonPagedBuffer(
        _Outptr_result_buffer_(Size) PVOID *Buffer,
        _In_ SIZE_T Size);

VOID IMFreeNonPagedBuffer(
    _Inout_ PVOID Buffer);

//
// Strings
//

_Check_return_
    NTSTATUS
    IMCopyString(
        _In_ PUNICODE_STRING Src,
        _Out_ PULONG DestSize,
        _Out_ PWCHAR *Dest,
        _Inout_ PULONG TotalSize);