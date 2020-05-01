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

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im_utils.h"

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMAllocateResource)
#pragma alloc_text(PAGE, IMFreeResource)
#pragma alloc_text(PAGE, IMAllocateNonPagedBuffer)
#pragma alloc_text(PAGE, IMFreeNonPagedBuffer)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

//
// Resources
//

_Check_return_
    NTSTATUS
    IMAllocateResource(
        _Outptr_ PERESOURCE *Resource)
{
  NTSTATUS status = STATUS_SUCCESS;
  PERESOURCE resource = NULL;
  BOOLEAN deleteResource = FALSE;
  BOOLEAN freeBuffer = FALSE;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Resource != NULL, STATUS_INVALID_PARAMETER_1);

  LOG(("[IM] Resource allocation\n"));

  __try
  {
    status = IMAllocateNonPagedBuffer((PVOID *)&resource, sizeof(ERESOURCE));
    if (!NT_SUCCESS(status))
    {
      freeBuffer = TRUE;
      __leave;
    }

    status = ExInitializeResourceLite(resource);
    if (!NT_SUCCESS(status))
    {
      deleteResource = TRUE;
      __leave;
    }

    *Resource = resource;
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] Resource allocation error\n"));

      if (deleteResource)
      {
        ExDeleteResourceLite(resource);
      }

      if (freeBuffer)
      {
        IMFreeNonPagedBuffer(resource);
      }
    }
  }

  LOG(("[IM] Resource 0x%p allocated\n", (PVOID) Resource));

  return status;
}

VOID IMFreeResource(
    _In_ PERESOURCE Resource)
{
  PAGED_CODE();

  IF_FALSE_RETURN(Resource != NULL);

  LOG(("[IM] Resource 0x%p freeing\n", (PVOID) Resource));

  ExDeleteResourceLite(Resource);
  IMFreeNonPagedBuffer(Resource);

  LOG(("[IM] Resource freed\n"));
}

//
// Buffers
//

_Check_return_
    NTSTATUS
    IMAllocateNonPagedBuffer(
        _Outptr_result_buffer_(Size) PVOID *Buffer,
        _In_ SIZE_T Size)
{
  PVOID allocatedBuffer = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Size != 0, STATUS_INVALID_PARAMETER_2);

  LOG(("[IM] Buffer of size 0x%x allocation\n", Size));

  allocatedBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, Size, IM_BUFFER_TAG);
  IF_FALSE_RETURN_RESULT(allocatedBuffer != NULL, STATUS_INSUFFICIENT_RESOURCES);

  // Zero buffer and set the result value.
  RtlZeroMemory(allocatedBuffer, Size);
  *Buffer = allocatedBuffer;

  LOG(("[IM] Buffer of size 0x%x allocated\n", Size));

  return STATUS_SUCCESS;
}

VOID IMFreeNonPagedBuffer(
    _Inout_ PVOID Buffer)
{
  PAGED_CODE();

  IF_FALSE_RETURN(Buffer != NULL);

  LOG(("[IM] Buffer freeing\n"));

  ExFreePool(Buffer);

  LOG(("[IM] Buffer freed\n"));
}