/*++

author:

Daulet Tumbayev

Module Name:

imlib_macro.h

Abstract:
Macro definitions here

Environment:

User mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Includes.
//------------------------------------------------------------------------

#include "stdio.h"

//------------------------------------------------------------------------
//  Debug macroses.
//------------------------------------------------------------------------

#if DBG
#define LOG(Data) printf Data;
#define LOG_B(Data) \
    printf Data;    \
    __debugbreak();
#else // DBG
#define LOG(Data) \
    {             \
          \
    }
#define LOG_B(Data) \
    {               \
            \
    }
#endif // DBG

//------------------------------------------------------------------------
//  Parameter validation macroses.
//------------------------------------------------------------------------

//
// returns , if the '_exp' expression is FALSE.
//
#define IF_FALSE_RETURN(_exp) \
    if (!(_exp))              \
    {                         \
        return;               \
    }

//
// returns the 'result', if the '_exp' expression is FALSE.
//
#define IF_FALSE_RETURN_RESULT(_exp, result) \
    if (!(_exp))                             \
    {                                        \
        return (result);                     \
    }

//
// Leaves the current '__try' block, if the '_exp' expression returns TRUE.
// Requires the 'HRESULT hResult' local variable to be defined.
//
#define HR_IF_TRUE_LEAVE(_exp, result) \
    if ((_exp))                        \
    {                                  \
        hResult = (result);            \
        __leave;                       \
    }

//
// Leaves the current '__try' block, if the '_exp' expression returns FALSE.
// Requires the 'HRESULT hResult' local variable to be defined.
//
#define HR_IF_FALSE_LEAVE(_exp, result) \
    if (!(_exp))                        \
    {                                   \
        hResult = (result);             \
        __leave;                        \
    }

//------------------------------------------------------------------------
//  Return value validation macroses.
//------------------------------------------------------------------------

//
// Leaves the current '__try' block, if the '_exp' expression returns a failing HRESULT.
// Requires the 'HRESULT hResult' local variable to be defined.
//
#define HR_IF_FAIL_LEAVE(_exp) \
    hResult = (_exp);          \
    if (FAILED(hResult))       \
    {                          \
        __leave;               \
    }

//
// Returns from the current function, if the '_exp' expression returns a failing HRESULT.
// Requires the 'HRESULT hResult' local variable to be defined.
//
#define HR_IF_FAIL_RETURN(_exp) \
    hResult = (_exp);           \
    if (FAILED(hResult))        \
    {                           \
        return hResult;         \
    }