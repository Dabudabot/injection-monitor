/*++

author:

Daulet Tumbayev

Module Name:

im_drv.c

Abstract:

This is the main for the mini-filter.

Environment:

Kernel mode

--*/

//---------------------------------------------------------------------------
//  Includes.
//---------------------------------------------------------------------------

#include "im_drv.h"
#include "im_comm.h"
#include "im_utils.h"
#include "im_rec.h"

//---------------------------------------------------------------------------
//  Assign text sections for each routine.
//---------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
// Functions that handle driver load/unload and instance setup/cleanup.
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, IMInstanceQueryTeardown)

// local funcstions
#pragma alloc_text(PAGE, IMInitializeGlobals)
#pragma alloc_text(PAGE, IMDeinitializeGlobals)
#endif

//
//  Global variable
//

IM_GLOBALS Globals;

//------------------------------------------------------------------------
//  Local function prototypes.
//------------------------------------------------------------------------

static _Check_return_
    NTSTATUS
    IMInitializeGlobals(
        _In_ PDRIVER_OBJECT DriverObject);

static VOID
IMDeinitializeGlobals();

//---------------------------------------------------------------------------
//  Main driver routines
//---------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
/*++

Routine Description:

This routine is called when a driver first loads.  Its purpose is to
initialize global state and then register with FltMgr to start filtering.

Arguments:

DriverObject - Pointer to driver object created by the system to
represent this driver.
RegistryPath - Unicode string identifying where the parameters for this
driver are located in the registry.

Return Value:

Status of the operation.

--*/
{
  NTSTATUS status = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(RegistryPath);

  FLT_ASSERT(DriverObject != NULL);

  LOG(("[IM] Driver loading\n"));

  __try
  {
    //
    // Initialize global data structures.
    //
    NT_IF_FAIL_LEAVE(IMInitializeGlobals(DriverObject));

    //
    //  Now that our global configuration is complete, register with FltMgr.
    //
    NT_IF_FAIL_LEAVE(FltRegisterFilter(DriverObject, &FilterRegistration, &Globals.Filter));

    //
    // Initialize communication
    //
    NT_IF_FAIL_LEAVE(IMInitCommunication(Globals.Filter, &Globals.ServerPort));

    //
    //  We are now ready to start filtering
    //
    NT_IF_FAIL_LEAVE(FltStartFiltering(Globals.Filter));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Driver loading failed\n"));

      if (NULL != Globals.ServerPort)
      {
        IMDeinitCommunication(Globals.ServerPort);
      }

      if (NULL != Globals.Filter)
      {
        FltUnregisterFilter(Globals.Filter);
      }

      IMDeinitializeGlobals();
    }
  }

  LOG(("[IM] Driver loaded\n"));

  return status;
}

NTSTATUS
FLTAPI
DriverUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags)
/*++

Routine Description:

This is called when a request has been made to unload the filter.  Unload
requests from the Operation System (ex: "sc stop imdrv" can not be
failed.  Other unload requests may be failed.

You can disallow OS unload request by setting the
FLTREGFL_DO_NOT_SUPPORT_SERVICE_STOP flag in the FLT_REGISTARTION
structure.

Arguments:

Flags - Flags pertinent to this operation

Return Value:

Always success

--*/
{
  UNREFERENCED_PARAMETER(Flags);

  PAGED_CODE();

  LOG(("[IM] Driver unloading\n"));

  if (NULL != Globals.ServerPort)
  {
    IMDeinitCommunication(Globals.ServerPort);
  }

  if (NULL != Globals.Filter)
  {
    FltUnregisterFilter(Globals.Filter);
  }

  IMDeinitializeGlobals();

  LOG(("[IM] Driver unloaded\n"));

  return STATUS_SUCCESS;
}

NTSTATUS
FLTAPI
IMInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
  UNREFERENCED_PARAMETER(FltObjects);
  UNREFERENCED_PARAMETER(Flags);
  PAGED_CODE();
  return STATUS_SUCCESS;
}

//
// -----------------------------------------------
//

static _Check_return_
    NTSTATUS
    IMInitializeGlobals(
        _In_ PDRIVER_OBJECT DriverObject)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_TRUE_RETURN_RESULT(DriverObject == NULL, STATUS_INVALID_PARAMETER);

  LOG(("[IM] Globals initializing\n"));

  Globals.DriverObject = DriverObject;
  Globals.LogSequenceNumber = 0;
  Globals.CSGOProcessId = NULL;
  Globals.CS16ProcessId = NULL;

  __try
  {
    NT_IF_FAIL_LEAVE(IMInitList(&Globals.RecordHead, sizeof(IM_KRECORD)));

    ExInitializeNPagedLookasideList(&Globals.RecordsLookaside,
                                    NULL,
                                    NULL,
                                    POOL_NX_ALLOCATION,
                                    sizeof(IM_KRECORD_LIST),
                                    IM_KRECORDS_TAG,
                                    0);

    NT_IF_FAIL_LEAVE(IMAllocateResource(&Globals.Lock));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Globals initialization error\n"));

      IMDeinitializeGlobals();
    }
  }

  LOG(("[IM] Globals initialized\n"));

  return status;
}

static VOID
IMDeinitializeGlobals()
{
  PAGED_CODE();

  LOG(("[IM] Globals deinitializing\n"));

  IMDeinitList(&Globals.RecordHead);

  ExDeleteNPagedLookasideList(&Globals.RecordsLookaside);

  if (NULL != Globals.Lock)
  {
    IMFreeResource(Globals.Lock);
  }

  LOG(("[IM] Globals deinitialized\n"));
}