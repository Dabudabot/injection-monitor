/*++

author:

Daulet Tumbayev

Module Name:

im_drv.h

Abstract:

Contains type definitions and function prototypes that are used
for mini-filter registration and operation.

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im.h"

//------------------------------------------------------------------------
//  Registration structure.
//------------------------------------------------------------------------

extern const FLT_REGISTRATION FilterRegistration;

//------------------------------------------------------------------------
//  Function prototypes.
//------------------------------------------------------------------------

//
//  Functions that handle driver load/unload
//

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
FLTAPI
DriverUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags);

//
// Instance functions
//

NTSTATUS
FLTAPI
IMInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);