// testdll1.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#define IMTEST_EXPORTS
#include "../include/TestHeader.h"
#include <iostream>


// This is an example of an exported function.
int TestFunction()
{
  std::cout << "hello from test dll 1" << std::endl;
    return 0;
}
