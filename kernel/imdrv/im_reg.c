/*++

author:

Daulet Tumbayev

Module Name:

im_reg.c

Abstract:

This filters registration information.  Note that this is in a unique file
so it could be set into the INIT section.

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im.h"
#include "im_ops.h"
#include "im_drv.h"

//------------------------------------------------------------------------
//  Registration structures.
//------------------------------------------------------------------------

//
//  Tells the compiler to define all following DATA and CONSTANT DATA to
//  be placed in the INIT segment.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#pragma const_seg("INIT")
#endif

//
//  Callbacks we are listening to
//  later we will add more
//
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

  { 
    IRP_MJ_CREATE,
    FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
    IMPreCreate,
    NULL,
    NULL 
  },

  { IRP_MJ_OPERATION_END,
  0, NULL, NULL, NULL }
};

//
//  This defines what we want to filter with FltMgr
//
CONST FLT_REGISTRATION FilterRegistration = {

  sizeof(FLT_REGISTRATION),               //  Size
  FLT_REGISTRATION_VERSION,               //  Version
  0,                                      //  Flags

  NULL,                               //  Context
  Callbacks,                          //  Operation callbacks

  DriverUnload,                       //  FilterUnload

  NULL,                               //  InstanceSetup
  IMInstanceQueryTeardown,            //  InstanceQueryTeardown
  NULL,                               //  InstanceTeardownStart
  NULL,                               //  InstanceTeardownComplete

  NULL,                               //  GenerateFileName
  NULL,                               //  GenerateDestinationFileName
  NULL,                               //  NormalizeNameComponent
  NULL,
  NULL
};


//
//  Tells the compiler to restore the given section types back to their previous
//  section definition.
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg()
#endif

