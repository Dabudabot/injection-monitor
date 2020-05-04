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

_Check_return_
    NTSTATUS
    IMInitCommunication(
        _In_ PFLT_FILTER Filter,
        _Outptr_ PFLT_PORT *Port);

VOID IMDeinitCommunication(
    _In_ PFLT_PORT Port);
