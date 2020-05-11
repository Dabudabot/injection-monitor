/*++

author:

Daulet Tumbayev

Module Name:

imapp.h

Abstract:
Simple app to test library

Environment:

User mode

--*/

#include "InjectorMonitor.h"
#include "Windows.h"
#include "stdio.h"
#include "stdlib.h"

HRESULT
RecordCallback(
    PIM_RECORD Record)
{
  wprintf(L"\n0x%x: \n  Process: %ls\n  Library: %ls\n  Blocked: %d Success: %d\n", Record->SequenceNumber, Record->ProcessName, Record->FileName, Record->IsBlocked, Record->IsSucceded);
  return S_OK;
}

int _cdecl main(
    _In_ int argc,
    _In_reads_(argc) char *argv[])
{
  HRESULT hResult;

  hResult = IMInitilize(RecordCallback);

  if (FAILED(hResult))
  {
    wprintf(L"Failed to initialize im lib\n");
    return 1;
  }

  system("pause");

  hResult = IMDeinitilize();

  if (FAILED(hResult))
  {
    wprintf(L"Failed to deinitialize im lib\n");
    return 1;
  }

  return 0;
}