
#include "Windows.h"
#include "TestHeader.h"

int _cdecl main(
    _In_ int argc,
    _In_reads_(argc) char *argv[])
{
  TestFunction();

  return 0;
}