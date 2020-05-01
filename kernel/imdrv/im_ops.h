/*++

author:

Daulet Tumbayev

Module Name:

im_ops.h

Abstract:
Header file which contains the function prototypes
of filtering callbacks

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

FLT_PREOP_CALLBACK_STATUS
FLTAPI
IMPreCreate(
_Inout_                        PFLT_CALLBACK_DATA    Data,
_In_                           PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID*                CompletionContext
);
