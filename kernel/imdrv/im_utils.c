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
#include "ntstrsafe.h"

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMAllocateNonPagedBuffer)
#pragma alloc_text(PAGE, IMFreeNonPagedBuffer)
#pragma alloc_text(PAGE, IMCopyString)
#pragma alloc_text(PAGE, IMAllocateUnicodeString)
#pragma alloc_text(PAGE, IMCopyUnicodeString)
#pragma alloc_text(PAGE, IMCopyUnicodeStringEx)
#pragma alloc_text(PAGE, IMIsContainsString)
#pragma alloc_text(PAGE, IMIsStartWithString)
#pragma alloc_text(PAGE, IMSplitString)
#pragma alloc_text(PAGE, IMConcatStrings)
#pragma alloc_text(PAGE, IMToString)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

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

  LOG(("[IM] String copy started\n"));

  if (NULL != *Dest)
  {
    // check for equality
    int result = wcscmp(Src->Buffer, *Dest);

    if (!result)
    {
      // already equal
      LOG(("[IM] Strings already equal\n"));
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

  LOG(("[IM] Strings copied\n"));

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

  NT_IF_FAIL_RETURN(IMAllocateNonPagedBuffer((PVOID *)&String->Buffer, Size));

  String->Length = 0;
  String->MaximumLength = Size;

  LOG(("[IM] String allocated\n"));

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

  LOG(("[IM] String to string copied\n"));

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

  NT_IF_FAIL_RETURN(IMAllocateUnicodeString(DestinationString, (USHORT)((Length + 1) * sizeof(WCHAR))));

  RtlCopyMemory(DestinationString->Buffer, (SourceString->Buffer + Start), Length * sizeof(WCHAR));
  RtlZeroMemory(DestinationString->Buffer + Length, sizeof(WCHAR)); // '\0'
  DestinationString->Length = (USHORT)(Length * sizeof(WCHAR));

  LOG(("[IM] String to string copied partially\n"));

  return STATUS_SUCCESS;
}

BOOLEAN
IMIsContainsString(
    _In_ PUNICODE_STRING String,
    _In_ PUNICODE_STRING SubString)
{
  ULONG i = 0;
  ULONG j = 0;
  WCHAR a;
  WCHAR b; // todo refactor

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(String->Buffer != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(String->Length != 0, FALSE);
  IF_FALSE_RETURN_RESULT(String->Length % sizeof(WCHAR) == 0, FALSE);

  IF_FALSE_RETURN_RESULT(SubString != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Buffer != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Length != 0, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Length % sizeof(WCHAR) == 0, FALSE);

  for (; i < String->Length / sizeof(WCHAR); i++)
  {
    a = String->Buffer[i];
    b = SubString->Buffer[j];

    if (a >= L'A' || a <= L'Z') a = a + (L'a' - L'A'); 
    if (b >= L'A' || b <= L'Z') b = b + (L'a' - L'A');

    if (a == b)
    {
      j++;
      if (SubString->Length / sizeof(WCHAR) == j)
      {
        LOG(("[IM] String contains string\n"));
        return TRUE;
      }
    }
    else
    {
      j = 0;
    }
  }

  LOG(("[IM] String not contains string\n"));

  return FALSE;
}

BOOLEAN
IMIsStartWithString(
    _In_ PUNICODE_STRING String,
    _In_ PUNICODE_STRING SubString)
{
  ULONG i = 0;
  ULONG j = 0;
  WCHAR a; // todo refactor
  WCHAR b;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(String != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(String->Buffer != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(String->Length != 0, FALSE);
  IF_FALSE_RETURN_RESULT(String->Length % sizeof(WCHAR) == 0, FALSE);

  IF_FALSE_RETURN_RESULT(SubString != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Buffer != NULL, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Length != 0, FALSE);
  IF_FALSE_RETURN_RESULT(SubString->Length % sizeof(WCHAR) == 0, FALSE);

  for (; i < String->Length / sizeof(WCHAR); i++)
  {
    a = String->Buffer[i];
    b = SubString->Buffer[j];

    if (a >= L'A' || a <= L'Z') a = a + (L'a' - L'A'); 
    if (b >= L'A' || b <= L'Z') b = b + (L'a' - L'A');

    if (String->Buffer[i] == SubString->Buffer[j])
    {
      j++;
      if (SubString->Length / sizeof(WCHAR) == j)
      {
        LOG(("[IM] String starts with string\n"));
        return TRUE;
      }
    }
    else
    {
      LOG(("[IM] String does not start with string\n"));
      return FALSE;
    }
  }

  LOG(("[IM] String does not start with string\n"));
  return FALSE;
}

// TODO refactor this function too many akward staff
_Check_return_
    NTSTATUS
    IMSplitString(
        _In_ PUNICODE_STRING String,
        _Out_opt_ PUNICODE_STRING Beginning,
        _Out_opt_ PUNICODE_STRING Ending,
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
  FLT_ASSERT(String->Length % sizeof(WCHAR) == 0);

  if (Beginning == NULL && Ending == NULL)
  {
    return STATUS_INVALID_PARAMETER;
  }

  if (Ending == NULL)
  {
    IF_FALSE_RETURN_RESULT(Beginning != NULL, STATUS_INVALID_PARAMETER_2);
    IF_FALSE_RETURN_RESULT(Beginning->Buffer == NULL, STATUS_INVALID_PARAMETER_2);
    IF_FALSE_RETURN_RESULT(Beginning->Length == 0, STATUS_INVALID_PARAMETER_2);
  }
  else
  {
    IF_FALSE_RETURN_RESULT(Ending->Buffer == NULL, STATUS_INVALID_PARAMETER_3);
    IF_FALSE_RETURN_RESULT(Ending->Length == 0, STATUS_INVALID_PARAMETER_3);
  }

  if (Beginning == NULL)
  {
    IF_FALSE_RETURN_RESULT(Ending != NULL, STATUS_INVALID_PARAMETER_3);
    IF_FALSE_RETURN_RESULT(Ending->Buffer == NULL, STATUS_INVALID_PARAMETER_3);
    IF_FALSE_RETURN_RESULT(Ending->Length == 0, STATUS_INVALID_PARAMETER_3);
  }
  else
  {
    IF_FALSE_RETURN_RESULT(Beginning->Buffer == NULL, STATUS_INVALID_PARAMETER_2);
    IF_FALSE_RETURN_RESULT(Beginning->Length == 0, STATUS_INVALID_PARAMETER_2);
  }

  LOG(("[IM] String splitting started\n"));

  if (String->Length == 0)
  {
    // original string is emptry, we also will create empty ones
    if (Beginning != NULL)
    {
      NT_IF_FAIL_RETURN(IMAllocateUnicodeString(Beginning, (USHORT) sizeof(WCHAR)));
      RtlZeroMemory(Beginning->Buffer, sizeof(WCHAR)); // '\0'
      Beginning->Length = 0;
    }

    if (Ending != NULL)
    {
      NT_IF_FAIL_RETURN(IMAllocateUnicodeString(Ending, (USHORT) sizeof(WCHAR)));
      RtlZeroMemory(Ending->Buffer, sizeof(WCHAR)); // '\0'
      Ending->Length = 0;
    }

    return STATUS_SUCCESS;
  }

  if (Occurrence < 0)
  {
    i = (String->Length / sizeof(WCHAR)) - 1;
    end = 0;
    step = -1;
    Occurrence *= (-1);
  }
  else
  {
    i = 0;
    end = (String->Length / sizeof(WCHAR)) - 1;
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

  // it was not found, we will just copy full string to beginning and nothing to ending
  if (i == end)
  {
    if (Beginning != NULL)
    {
      NT_IF_FAIL_RETURN(IMCopyUnicodeString(Beginning, String));
    }

    if (Ending != NULL)
    {
      NT_IF_FAIL_RETURN(IMAllocateUnicodeString(Ending, (USHORT) sizeof(WCHAR)));
      RtlZeroMemory(Ending->Buffer, sizeof(WCHAR)); // '\0'
      Ending->Length = 0;
    }
    
    return STATUS_SUCCESS;
  }

  // now i is the first symbol of the ending string
  if (Beginning != NULL)
  {
    NT_IF_FAIL_RETURN(IMCopyUnicodeStringEx(Beginning, String, 0, i));
  }

  if (Ending != NULL)
  {
    NT_IF_FAIL_RETURN(IMCopyUnicodeStringEx(Ending, String, i, (String->Length / sizeof(WCHAR)) - i));
  }

  LOG(("[IM] String splitted\n"));

  return status;
}

_Check_return_
    NTSTATUS
    IMConcatStrings(
        _Out_ PUNICODE_STRING Dest,
        _In_ PUNICODE_STRING Start,
        _In_ PUNICODE_STRING End)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Dest != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Dest->Buffer == NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Dest->Length == 0, STATUS_INVALID_PARAMETER_2);

  IF_FALSE_RETURN_RESULT(Start != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Start->Buffer != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(Start->Length != 0, STATUS_INVALID_PARAMETER_2);

  IF_FALSE_RETURN_RESULT(End != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(End->Buffer != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(End->Length != 0, STATUS_INVALID_PARAMETER_3);

  NT_IF_FAIL_RETURN(IMAllocateUnicodeString(Dest, Start->Length + End->Length + sizeof(WCHAR)));

  RtlCopyMemory(Dest->Buffer, Start->Buffer, Start->Length);
  RtlCopyMemory(Dest->Buffer + (Start->Length / sizeof(WCHAR)), End->Buffer, End->Length);
  RtlZeroMemory(Dest->Buffer + ((Start->Length + End->Length) / sizeof(WCHAR)), sizeof(WCHAR)); // '/0'

  Dest->Length = Start->Length + End->Length;

  return STATUS_SUCCESS;
}

_Check_return_
    NTSTATUS
    IMToString(
        _In_ PWCHAR Buffer,
        _In_ ULONG Size,
        _Out_ PUNICODE_STRING String)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Buffer != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Size != 0, STATUS_INVALID_PARAMETER_2);

  IF_FALSE_RETURN_RESULT(String != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(String->Buffer == NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(String->Length == 0, STATUS_INVALID_PARAMETER_3);

  NT_IF_FAIL_RETURN(IMAllocateUnicodeString(String, (USHORT) Size));
  RtlCopyMemory(String->Buffer, Buffer, Size - sizeof(WCHAR));
  RtlZeroMemory(String->Buffer + (Size / sizeof(WCHAR)) - 1, sizeof(WCHAR)); // '/0'
  String->Length = (USHORT) (Size - sizeof(WCHAR));

  return STATUS_SUCCESS;
}