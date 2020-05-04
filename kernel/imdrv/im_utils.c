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
#pragma alloc_text(PAGE, IMCopyString)
#pragma alloc_text(PAGE, IMAllocateUnicodeString)
#pragma alloc_text(PAGE, IMCopyUnicodeString)
#pragma alloc_text(PAGE, IMCopyUnicodeStringEx)
#pragma alloc_text(PAGE, IMIsContainsString)
#pragma alloc_text(PAGE, IMIsStartWithString)
#pragma alloc_text(PAGE, IMSplitString)
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

  LOG(("[IM] Resource 0x%p allocated\n", (PVOID)Resource));

  return status;
}

VOID IMFreeResource(
    _In_ PERESOURCE Resource)
{
  PAGED_CODE();

  IF_FALSE_RETURN(Resource != NULL);

  LOG(("[IM] Resource 0x%p freeing\n", (PVOID)Resource));

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

_Check_return_
    _IRQL_requires_(PASSIVE_LEVEL)
        NTSTATUS
    IMCopyString(
        _In_ PUNICODE_STRING Src,
        _Out_ PULONG DestSize,
        _Out_ PVOID *Dest,
        _Inout_ PULONG TotalSize)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Src != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Src->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Src->Length != 0, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(DestSize != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Dest != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(TotalSize != NULL, STATUS_INVALID_PARAMETER_4);
  IF_FALSE_RETURN_RESULT(KeGetCurrentIrql() == PASSIVE_LEVEL, STATUS_REQUEST_NOT_ACCEPTED);

  if (NULL != *Dest)
  {
    // check for equality
    int result = wcscmp(Src->Buffer, *Dest);

    if (!result)
    {
      // already equal
      return STATUS_SUCCESS;
    }

    ExFreePool(*Dest);
    *TotalSize -= (*DestSize);
    *DestSize = 0;
  }

  NT_IF_FAIL_RETURN(IMAllocateNonPagedBuffer(Dest, Src->Length + 2));

  *DestSize = Src->Length + 2;
  *TotalSize += (*DestSize);

  RtlCopyMemory(*Dest, Src->Buffer, Src->Length);
  RtlZeroMemory((PCHAR)(*Dest) + Src->Length, 2); // just in case in order to null terminate

  return STATUS_SUCCESS;
}

_Check_return_
    NTSTATUS
    IMAllocateUnicodeString(
        _Inout_ PUNICODE_STRING String,
        _In_ USHORT Size)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Buffer == NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Size != 0, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Size % sizeof(WCHAR) == 0, STATUS_INVALID_PARAMETER_2);

  NT_IF_FAIL_RETURN(IMAllocateNonPagedBuffer((PVOID *)&String->Buffer, Size));

  String->Length = 0;
  String->MaximumLength = Size;

  // The buffer allocated should be freed by the caller.

  return status;
}

_Check_return_
    NTSTATUS
    IMCopyUnicodeString(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(DestinationString != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(DestinationString->Buffer == NULL, STATUS_INVALID_PARAMETER_1);

  IF_FALSE_RETURN_RESULT(SourceString->Buffer != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(NT_SUCCESS(RtlUnicodeStringValidate(SourceString)), STATUS_INVALID_PARAMETER_2);

  NT_IF_FAIL_RETURN(IMAllocateUnicodeString(DestinationString, min(SourceString->Length, SourceString->MaximumLength) + sizeof(WCHAR)));

#pragma warning(suppress \
                : __WARNING_INVALID_PARAM_VALUE_1) // '_Param_(1)->Buffer' could be '0'.
  RtlCopyUnicodeString(DestinationString, SourceString);

  return status;
}

_Check_return_
    NTSTATUS
    IMCopyUnicodeStringEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ ULONG Start,
        _In_ ULONG Length)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(DestinationString != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(DestinationString->Buffer == NULL, STATUS_INVALID_PARAMETER_1);

  IF_FALSE_RETURN_RESULT(SourceString->Buffer != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(NT_SUCCESS(RtlUnicodeStringValidate(SourceString)), STATUS_INVALID_PARAMETER_2);

  NT_IF_FAIL_RETURN(IMAllocateUnicodeString(DestinationString, Length + sizeof(WCHAR)));

  RtlCopyMemory(DestinationString->Buffer, (SourceString->Buffer + Start), Length * sizeof(WCHAR));
  RtlZeroMemory(DestinationString->Buffer + Length, sizeof(WCHAR)); // '\0'
  DestinationString->Length = Length;

  return STATUS_SUCCESS;
}

BOOLEAN
IMIsContainsString(
    _In_ PUNICODE_STRING String,
    _In_ PUNICODE_STRING SubString)
{
  ULONG i = 0;
  ULONG j = 0;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Length != 0, STATUS_INVALID_PARAMETER_1);

  IF_FALSE_RETURN_RESULT(SubString != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(SubString->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(SubString->Length != 0, STATUS_INVALID_PARAMETER_1);

  for (; i < String->Length; i++)
  {
    if (String->Buffer[i] == SubString->Buffer[j])
    {
      j++;
      if (SubString->Length == j)
      {
        return TRUE;
      }
    }
    else
    {
      j = 0;
    }
  }

  return FALSE;
}

BOOLEAN
IMIsStartWithString(
    _In_ PUNICODE_STRING String,
    _In_ PUNICODE_STRING SubString)
{
  ULONG i = 0;
  ULONG j = 0;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Length != 0, STATUS_INVALID_PARAMETER_1);

  IF_FALSE_RETURN_RESULT(SubString != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(SubString->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(SubString->Length != 0, STATUS_INVALID_PARAMETER_1);

  for (; i < String->Length; i++)
  {
    if (String->Buffer[i] == SubString->Buffer[j])
    {
      j++;
      if (SubString->Length == j)
      {
        return TRUE;
      }
    }
    else
    {
      return FALSE;
    }
  }

  return FALSE;
}

_Check_return_
    NTSTATUS
    IMSplitString(
        _In_ PUNICODE_STRING String,
        _Outptr_ PUNICODE_STRING Beginning,
        _Outptr_ PUNICODE_STRING Ending,
        _In_ WCHAR Delimeter,
        _In_ LONG Occurrence)
{
  NTSTATUS status = STATUS_SUCCESS;
  ULONG i = 0;
  LONG step = 1;
  ULONG end = 0;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(String->Length != 0, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Beginning != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Beginning->Buffer == NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Beginning->Length == 0, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Ending != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(Ending->Buffer == NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(Ending->Length == 0, STATUS_INVALID_PARAMETER_3);

  if (Occurrence < 0)
  {
    i = String->Length - 1;
    end = 0;
    step = -1;
    Occurrence *= (-1);
  }
  else
  {
    i = 0;
    end = String->Length - 1;
    step = 1;
  }

  for (; i != end; i += step)
  {
    if (String->Buffer[i] == Delimeter)
    {
      Occurrence--;
      if (Occurrence == 0)
      {
        i++;
        break;
      }
    }
  }

  // now i is the first symbol of the ending string

  NT_IF_FAIL_RETURN(IMCopyUnicodeStringEx(Beginning, String, 0, i));

  NT_IF_FAIL_RETURN(IMCopyUnicodeStringEx(Ending, String, i, String->Length - i));
  
  return status;
}