/*++

author:

Daulet Tumbayev

Module Name:

im_rec.h

Abstract:

Record control function definition and structures

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im_rec.h"
#include "im_utils.h"

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMInitList)
#pragma alloc_text(PAGE, IMDeinitList)
#pragma alloc_text(PAGE, IMFreeList)
#pragma alloc_text(PAGE, IMPush)
#pragma alloc_text(PAGE, IMPushToList)
#pragma alloc_text(PAGE, IMPop)
#pragma alloc_text(PAGE, IMCreateRecord)
#pragma alloc_text(PAGE, IMFreeRecord)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

//
// List manipulation
//

_Check_return_
    NTSTATUS
    IMInitList(
        _Inout_ PIM_KRECORD_HEAD RecordHead,
        _In_ SIZE_T Size)
{
  NTSTATUS status = STATUS_SUCCESS;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(RecordHead != NULL, STATUS_INVALID_PARAMETER_1);
  IF_FALSE_RETURN_RESULT(Size != 0, STATUS_INVALID_PARAMETER_2);

  LOG(("[IM] List initializing\n"));

  __try
  {
    RecordHead->MaxRecordsToPush = IM_DEFAULT_MAX_RECORDS;
    RecordHead->RecordsPushed = 0;
    RecordHead->RecordStructSize = Size;
    NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&RecordHead->OutputRecordEvent, sizeof(KEVENT)));

    KeInitializeEvent(
        RecordHead->OutputRecordEvent,
        NotificationEvent,
        FALSE);

    InitializeListHead(&RecordHead->RecordList);
    KeInitializeSpinLock(&RecordHead->RecordListLock);
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] List initializing error\n"));

      IMDeinitList(RecordHead);
    }
  }

  LOG(("[IM] List initialized\n"));

  return STATUS_SUCCESS;
}

VOID IMDeinitList(
    _Inout_ PIM_KRECORD_HEAD RecordHead)
{
  PAGED_CODE();

  IF_FALSE_RETURN(RecordHead != NULL);
  IF_FALSE_RETURN(RecordHead->OutputRecordEvent != NULL);

  LOG(("[IM] List deinitializing\n"));

  IMFreeList(&RecordHead->RecordListLock, &RecordHead->RecordList);

  ExFreePool(RecordHead->OutputRecordEvent);

  LOG(("[IM] List deinitialized\n"));
}

VOID IMFreeList(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead)
{
  PLIST_ENTRY recordListEntry = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN(ListHead != NULL);
  IF_FALSE_RETURN(Lock != NULL);

  LOG(("[IM] List freeing\n"));

  IMPop(Lock, ListHead, &recordListEntry);

  //  iterate over list
  while (recordListEntry != NULL)
  {
    PIM_KRECORD_LIST recordList = CONTAINING_RECORD(recordListEntry, IM_KRECORD_LIST, List);

    IMFreeRecord(recordList);

    IMPop(Lock, ListHead, &recordListEntry);
  }

  LOG(("[IM] List freed\n"));
}

VOID IMPush(
    _In_ PLIST_ENTRY ListEntry,
    _In_ PIM_KRECORD_HEAD RecordsHead)
{
  PAGED_CODE();

  IF_FALSE_RETURN(ListEntry != NULL);
  IF_FALSE_RETURN(RecordsHead != NULL);

  ARPushToList(
      &RecordsHead->RecordListLock,
      &RecordsHead->RecordList,
      ListEntry,
      RecordsHead->OutputRecordEvent);

  InterlockedIncrement64(&RecordsHead->RecordsPushed);
}

VOID IMPushToList(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY ListEntry,
    _In_opt_ PKEVENT Event)
{
  KIRQL oldIrql;
  PAGED_CODE();

  IF_FALSE_RETURN(SpinLock != NULL);
  IF_FALSE_RETURN(ListHead != NULL);
  IF_FALSE_RETURN(ListEntry != NULL);
  IF_FALSE_RETURN(Event != NULL);

  KeAcquireSpinLock(SpinLock, &oldIrql);

  InsertTailList(ListHead, ListEntry);

  KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

  KeReleaseSpinLock(SpinLock, oldIrql);
}

VOID IMPop(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _Outptr_ PLIST_ENTRY *RecordListEntry)
{
  KIRQL oldIrql;

  PAGED_CODE();

  *RecordListEntry = NULL;

  IF_FALSE_RETURN(ListHead != NULL);
  IF_FALSE_RETURN(Lock != NULL);

  KeAcquireSpinLock(Lock, &oldIrql);

  if (!IsListEmpty(ListHead))
  {
    *RecordListEntry = RemoveHeadList(ListHead);
  }

  KeReleaseSpinLock(Lock, oldIrql);
}

_Check_return_
    _IRQL_requires_max_(APC_LEVEL)
        NTSTATUS
    IMCreateRecord(
        _Outptr_ PIM_KRECORD_LIST *RecordList,
        _In_ PFLT_CALLBACK_DATA Data,
        _In_ PUNICODE_STRING FileName)
{
  NTSTATUS status = STATUS_SUCCESS;
  PIM_KRECORD_LIST newRecord = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Data != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(FileName != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(KeGetCurrentIrql() <= APC_LEVEL, STATUS_UNSUCCESSFUL);
  IF_FALSE_RETURN_RESULT(Globals.RecordHead.RecordsPushed < Globals.RecordHead.MaxRecordsToPush, STATUS_MAX_REFERRALS_EXCEEDED);

  __try
  {

    newRecord = (PIM_KRECORD_LIST)ExAllocateFromNPagedLookasideList(&Globals.RecordsLookaside);

    if (NULL == newRecord)
    {
      status = STATUS_INSUFFICIENT_RESOURCES;
      __leave;
    }

    RtlZeroMemory(&newRecord->Record, sizeof(IM_KRECORD));

    //  setting data
    newRecord->Record.Debug = 0xCEFAADDE;
    newRecord->Record.TotalLength = sizeof(IM_KRECORD);
    KeQuerySystemTime(&newRecord->Record.Time);

    NT_IF_FAIL_LEAVE(IMCopyString(FileName, &newRecord->Record.NameSize, &newRecord->Record.Name, &newRecord->Record.TotalLength));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      if (NULL != newRecord)
      {
        IMFreeRecord(newRecord);
      }
    }
    else
    {
      newRecord->Record.SequenceNumber = InterlockedIncrement64(&Globals.LogSequenceNumber); // todo may overrun
      *RecordList = newRecord;
    }
  }

  return status;
}

VOID IMFreeRecord(
    _In_ PIM_KRECORD_LIST RecordList)
{
  PAGED_CODE();

  IF_FALSE_RETURN(RecordList != NULL);

  IMFreeNonPagedBuffer((PVOID)RecordList->Record.Name);

  ExFreeToNPagedLookasideList(&Globals.RecordsLookaside, RecordList);
}