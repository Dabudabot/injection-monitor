/*++

author:

Daulet Tumbayev

Module Name:

InjectorMonitorCommon.h

Abstract:
Common structs for user, kernel and client

Environment:

User mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Structures.
//------------------------------------------------------------------------

typedef enum _IM_VIDEO_MODE_STATUS
{
  
  IM_NOT_APPLICABLE,
  IM_VIDEO_ERROR,    // error during determmining or changing video mode
  IM_VIDEO_SW,       // game loaded in software
  IM_VIDEO_HW,       // game loaded in hardware
  IM_VIDEO_SW_TO_HW, // game tried to load in software but finally loaded in hardware
  IM_VIDEO_HW_TO_SW  // game tried to load in hardware but finally loaded in hardware

} IM_VIDEO_MODE_STATUS,
    *PIM_VIDEO_MODE_STATUS;