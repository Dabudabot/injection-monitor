/*++

author:

Daulet Tumbayev

Module Name:

im_list.c

Abstract:

List implementation

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "im_list.h"
#include "im_utils.h"

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMInitList)
#pragma alloc_text(PAGE, IMDeinitList)
#pragma alloc_text(PAGE, IMFreeList)
#pragma alloc_text(PAGE, IMPush)
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
        _Inout_ PIM_KLIST_HEAD ListHead,
        _In_ SIZE_T Size,
        _In_ LONG MaxElementsToPush,
        _In_ IM_KELEMENT_FREE_CALLBACK ElementFreeCallback)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    IF_FALSE_RETURN_RESULT(ListHead != NULL, STATUS_INVALID_PARAMETER_1);
    IF_FALSE_RETURN_RESULT(Size != 0, STATUS_INVALID_PARAMETER_2);

    LOG(("[IM] List initializing\n"));

    __try
    {
        ListHead->MaxElementsToPush = MaxElementsToPush;
        ListHead->ElementsPushed = 0;
        ListHead->ElementStructSize = (ULONG)Size;
        ListHead->ElementFreeCallback = ElementFreeCallback;
        NT_IF_FAIL_LEAVE(IMAllocateNonPagedBuffer(&ListHead->NewElementEvent, sizeof(KEVENT)));

        KeInitializeEvent(
            ListHead->NewElementEvent,
            NotificationEvent,
            FALSE);

        InitializeListHead(&ListHead->ElementList);
        KeInitializeSpinLock(&ListHead->ElementListLock);

        ExInitializeNPagedLookasideList(&ListHead->ElementsLookaside,
                                        NULL,
                                        NULL,
                                        POOL_NX_ALLOCATION,
                                        Size + sizeof(LIST_ENTRY),
                                        IM_KLIST_TAG,
                                        0);
    }
    __finally
    {
        if (NT_ERROR(status))
        {
            LOG_B(("[IM] List initializing error\n"));

            IMDeinitList(ListHead);
        }
        else
        {
            LOG(("[IM] List initialized\n"));
        }
    }

    return STATUS_SUCCESS;
}

VOID IMDeinitList(
    _Inout_ PIM_KLIST_HEAD ListHead)
{
    PAGED_CODE();

    IF_FALSE_RETURN(ListHead != NULL);
    IF_FALSE_RETURN(ListHead->NewElementEvent != NULL);

    LOG(("[IM] List deinitializing\n"));

    IMFreeList(&ListHead->ElementListLock, &ListHead->ElementList, ListHead->ElementFreeCallback);

    ExFreePool(ListHead->NewElementEvent);

    ExDeleteNPagedLookasideList(&ListHead->ElementsLookaside);

    LOG(("[IM] List deinitialized\n"));
}

VOID IMFreeList(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _In_ IM_KELEMENT_FREE_CALLBACK ElementFreeCallback)
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
        ElementFreeCallback(recordListEntry);

        IMPop(Lock, ListHead, &recordListEntry);
    }

    LOG(("[IM] List freed\n"));
}

VOID IMPush(
    _In_ PLIST_ENTRY ListEntry,
    _In_ PIM_KLIST_HEAD ListHead)
{
    PAGED_CODE();

    IF_FALSE_RETURN(ListEntry != NULL);
    IF_FALSE_RETURN(ListHead != NULL);

    InterlockedIncrement64(&ListHead->ElementsPushed);

    IMPushToList(
        &ListHead->ElementListLock,
        &ListHead->ElementList,
        ListEntry,
        ListHead->NewElementEvent);
}

VOID IMPushToList(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY ListEntry,
    _In_opt_ PKEVENT Event)
{
    KIRQL oldIrql;

    IF_FALSE_RETURN(SpinLock != NULL);
    IF_FALSE_RETURN(ListHead != NULL);
    IF_FALSE_RETURN(ListEntry != NULL);
    IF_FALSE_RETURN(Event != NULL);

    KeAcquireSpinLock(SpinLock, &oldIrql);

    InsertTailList(ListHead, ListEntry);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(SpinLock, oldIrql);

    LOG(("[IM] Element pushed to list\n"));
}

VOID IMPop(
    _In_ PKSPIN_LOCK Lock,
    _In_ PLIST_ENTRY ListHead,
    _Outptr_ PLIST_ENTRY *ListEntry)
{
    KIRQL oldIrql;

    *ListEntry = NULL;

    IF_FALSE_RETURN(ListHead != NULL);
    IF_FALSE_RETURN(Lock != NULL);

    KeAcquireSpinLock(Lock, &oldIrql);

    if (!IsListEmpty(ListHead))
    {
        *ListEntry = RemoveHeadList(ListHead);
    }

    KeReleaseSpinLock(Lock, oldIrql);

    LOG(("[IM] Element poped\n"));
}