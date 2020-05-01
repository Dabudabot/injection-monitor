/*++

author:

Daulet Tumbayev

Module Name:

im_comm.h

Abstract:

Communication with user space definitions

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im.h"
#include "im_rec.h"

//------------------------------------------------------------------------
//  Function prototypes.
//------------------------------------------------------------------------

//
// Communcation control
//

NTSTATUS
IMInitCommunication(
    _In_ PFLT_FILTER Filter,
    _Outptr_ PFLT_PORT *Port);

NTSTATUS
IMDeinitCommunication(
    _In_ PFLT_PORT Port);

NTSTATUS
IMConnect(
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie);

VOID IMDisconnect(
    _In_opt_ PVOID ConnectionCookie);

NTSTATUS
IMMessage(
    _In_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength);

LONG IMExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointer,
    _In_ BOOLEAN AccessingUserBuffer);

//
// Message functions
//

NTSTATUS
IMGetRecords(
    _In_ PIM_KRECORD_HEAD RecordHead,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength);