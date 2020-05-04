/*++

author:

Daulet Tumbayev

Module Name:

im_macro.h

Abstract:
Macro definitions here

Environment:

Kernel mode

--*/

#pragma once

//------------------------------------------------------------------------
//  Debug macroses.
//------------------------------------------------------------------------

#if DBG
#define LOG(Data) DbgPrint Data;
#define LOG_B(Data) \
    DbgPrint Data;  \
    DbgBreakPoint();
#else // DBG
#define LOG(Data) \
    {             \
        NOTHING;  \
    }
#define LOG_B(Data) \
    {               \
        NOTHING;    \
    }
#endif // DBG

//------------------------------------------------------------------------
//  Parameter validation macroses.
//------------------------------------------------------------------------

//
// Calls the 'FLT_ASSERTMSG' and returns from the current function, if the '_exp' expression is FALSE.
//
#define IF_FALSE_RETURN(_exp)        \
    if (!(_exp))                     \
    {                                \
        FLT_ASSERTMSG(#_exp, FALSE); \
        return;                      \
    }

//
// Calls the 'FLT_ASSERTMSG' and returns from the current function, if the '_exp' expression is TRUE.
//
#define IF_TRUE_RETURN(_exp)         \
    if ((_exp))                      \
    {                                \
        FLT_ASSERTMSG(#_exp, FALSE); \
        return;                      \
    }

//
// Calls the 'FLT_ASSERTMSG' and returns the 'result', if the '_exp' expression is FALSE.
//
#define IF_FALSE_RETURN_RESULT(_exp, result) \
    if (!(_exp))                             \
    {                                        \
        FLT_ASSERTMSG(#_exp, FALSE);         \
        return (result);                     \
    }

//
// Calls the 'FLT_ASSERTMSG' and returns the 'result', if the '_exp' expression is TRUE.
//
#define IF_TRUE_RETURN_RESULT(_exp, result) \
    if ((_exp))                             \
    {                                       \
        FLT_ASSERTMSG(#_exp, FALSE);        \
        return (result);                    \
    }

//
// Leaves the current '__try' block, if the '_exp' expression returns TRUE.
// Requires the 'NTSTATUS status' local variable to be defined.
//
#define NT_IF_TRUE_LEAVE(_exp, result) \
    if ((_exp))                        \
    {                                  \
        status = (result);             \
        __leave;                       \
    }

//
// Leaves the current '__try' block, if the '_exp' expression returns FALSE.
// Requires the 'NTSTATUS status' local variable to be defined.
//
#define NT_IF_FALSE_LEAVE(_exp, result) \
    if (!(_exp))                        \
    {                                   \
        status = (result);              \
        __leave;                        \
    }

//------------------------------------------------------------------------
//  Return value validation macroses.
//------------------------------------------------------------------------

//
// Leaves the current '__try' block, if the '_exp' expression returns a failing NTSTATUS.
// Requires the 'NTSTATUS status' local variable to be defined.
//
#define NT_IF_FAIL_LEAVE(_exp) \
    status = (_exp);           \
    if (!NT_SUCCESS(status))   \
    {                          \
        __leave;               \
    }

//
// Returns from the current function, if the '_exp' expression returns a failing NTSTATUS.
// Requires the 'NTSTATUS status' local variable to be defined.
//
#define NT_IF_FAIL_RETURN(_exp) \
    status = (_exp);            \
    if (!NT_SUCCESS(status))    \
    {                           \
        return status;          \
    }

//
// Creates a new UNICODE_STRING.
//
#define CONSTANT_STRING(x)                             \
    {                                                  \
        sizeof((x)) - sizeof((x)[0]), sizeof((x)), (x) \
    }