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
#pragma alloc_text(PAGE, IMCreateRecord)
#pragma alloc_text(PAGE, IMFreeRecord)
#pragma alloc_text(PAGE, IMFreeRecordList)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Functions.
//------------------------------------------------------------------------

_Check_return_
    _IRQL_requires_max_(APC_LEVEL)
        NTSTATUS
    IMCreateRecord(
        _Outptr_ PIM_KRECORD_LIST *RecordList,
        _In_ PFLT_CALLBACK_DATA Data,
        _In_ PIM_NAME_INFORMATION FileNameInfo,
        _In_ PUNICODE_STRING ProcessName,
        _In_ IM_VIDEO_MODE_STATUS VideoMode)
{
  NTSTATUS status = STATUS_SUCCESS;
  PIM_KRECORD_LIST newRecord = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN_RESULT(Data != NULL, STATUS_INVALID_PARAMETER_2);
  IF_FALSE_RETURN_RESULT(FileNameInfo != NULL, STATUS_INVALID_PARAMETER_3);
  IF_FALSE_RETURN_RESULT(KeGetCurrentIrql() <= APC_LEVEL, STATUS_UNSUCCESSFUL);
  IF_FALSE_RETURN_RESULT(Globals.RecordsHead.ElementsPushed < Globals.RecordsHead.MaxElementsToPush, STATUS_MAX_REFERRALS_EXCEEDED);

  LOG(("[IM] Record creation start\n"));

  __try
  {

    newRecord = (PIM_KRECORD_LIST)ExAllocateFromNPagedLookasideList(&Globals.RecordsHead.ElementsLookaside);

    if (NULL == newRecord)
    {
      status = STATUS_INSUFFICIENT_RESOURCES;
      LOG(("[IM] INSUFFICIENT resources to create record\n"));
      __leave;
    }

    RtlZeroMemory(&newRecord->Record, sizeof(IM_KRECORD));

    //  setting data
    newRecord->Record.Debug = 0xCEFAADDE;
    newRecord->Record.TotalLength = sizeof(IM_KRECORD);
    newRecord->Record.VideoModeStatus = VideoMode;
    newRecord->Record.FileNameInformation = FileNameInfo;
    KeQuerySystemTime(&newRecord->Record.Time);

    NT_IF_FAIL_LEAVE(IMCopyString(&FileNameInfo->FullName, &newRecord->Record.Data[IM_FILE_NAME_INDEX].Size, &newRecord->Record.Data[IM_FILE_NAME_INDEX].Buffer, &newRecord->Record.TotalLength));

    NT_IF_FAIL_LEAVE(IMCopyString(ProcessName, &newRecord->Record.Data[IM_PROCESS_NAME_INDEX].Size, &newRecord->Record.Data[IM_PROCESS_NAME_INDEX].Buffer, &newRecord->Record.TotalLength));
  }
  __finally
  {
    if (NT_ERROR(status))
    {
      LOG_B(("[IM] record creation failed\n"));
      if (NULL != newRecord)
      {
        IMFreeRecord(newRecord);
      }
    }
    else
    {
      newRecord->Record.SequenceNumber = InterlockedIncrement64(&Globals.RecordsHead.SequenceNumber); // todo may overrun
      *RecordList = newRecord;
      LOG(("[IM] Record created with %wZ and %wZ\n", ProcessName, &FileNameInfo->FullName));
    }
  }

  return status;
}

VOID IMFreeRecord(
    _In_ PIM_KRECORD_LIST RecordList)
{
  PAGED_CODE();

  IF_FALSE_RETURN(RecordList != NULL);

  for (ULONG i = 0; i < IM_AMOUNT_OF_DATA; i++)
  {
    IMFreeNonPagedBuffer((PVOID)RecordList->Record.Data[i].Buffer);
  }

  ExFreeToNPagedLookasideList(&Globals.RecordsHead.ElementsLookaside, RecordList);

  LOG(("[IM] Record freed\n"));
}

VOID IMFreeRecordList(
    _In_ PLIST_ENTRY ListEntry)
{
  PIM_KRECORD_LIST recordList = NULL;

  PAGED_CODE();

  IF_FALSE_RETURN(ListEntry != NULL);

  recordList = CONTAINING_RECORD(ListEntry, IM_KRECORD_LIST, List);

  IMFreeRecord(recordList);
}