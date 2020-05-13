/*++

author:

Daulet Tumbayev

Module Name:

im_proc.c

Abstract:
Process callbacks

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------

#include "im_proc.h"
#include "im_req.h"
#include "im_utils.h"

//------------------------------------------------------------------------
//  Defines.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  Text sections.
//------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMCreateProcessNotifyRoutine)
#endif // ALLOC_PRAGMA

//------------------------------------------------------------------------
//  Function prototypes
//------------------------------------------------------------------------

VOID IMCreateProcessNotifyRoutine(
    HANDLE ParentId,
    HANDLE ProcessId,
    BOOLEAN Create)
{
  UNREFERENCED_PARAMETER(ParentId);

  NTSTATUS status = STATUS_SUCCESS;
  PIM_NAME_INFORMATION processNameInfo = NULL;
  ULONG i = 0;
  PIM_PROCESS_INFO target = NULL;
  BOOLEAN isFound = FALSE;

  PAGED_CODE();

  IF_FALSE_RETURN(ProcessId != NULL);

  __try
  {
    NT_IF_FAIL_LEAVE(IMGetProcessNameInformation(ProcessId, &processNameInfo));

    for (; i < IM_AMOUNT_OF_TARGET_PROCESSES; i++)
    {
      target = &Globals.TargetProcessInfo[i];
      if (RtlCompareUnicodeString(&processNameInfo->Name, &target->TargetName, TRUE) == 0)
      {
        if (Create)
        {
          isFound = TRUE;

          if (target->isActive)
          {
            LOG_B(("[IM] PROCESS DUPLICATION\n")); // TODO
            target->isDuplicate = TRUE;
            IMReleaseNameInformation(target->NameInfo);
          }

          target->NameInfo = processNameInfo;
          target->isActive = TRUE;
          target->ProcessId = ProcessId;

          LOG(("[IM] Found process creation: %wZ\n", &processNameInfo->Name));
        }
        else
        {
          LOG(("[IM] Found process termination: %wZ\n", &processNameInfo->Name));
          IMReleaseNameInformation(target->NameInfo);
          target->isActive = FALSE;
          target->isDuplicate = FALSE;
          target->ProcessId = NULL;
        }
      }
    }
  }
  __finally
  {
    if (!isFound && processNameInfo != NULL)
    {
      IMReleaseNameInformation(processNameInfo);
    }
  }
}