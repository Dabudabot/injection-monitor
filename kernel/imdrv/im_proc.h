/*++

author:

Daulet Tumbayev

Module Name:

im_proc.h

Abstract:
Process callbacks

Environment:

Kernel mode

--*/

//------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------

#include "im.h"
#include "im_req.h"

//------------------------------------------------------------------------
//  Function prototypes
//------------------------------------------------------------------------

VOID IMCreateProcessNotifyRoutine(
    HANDLE ParentId,
    HANDLE ProcessId,
    BOOLEAN Create);