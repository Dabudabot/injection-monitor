#pragma once

#ifdef IMTEST_EXPORTS
#define IMTEST_API __declspec(dllexport)
#else
#define IMTEST_API __declspec(dllimport)
#endif

extern "C" IMTEST_API int TestFunction();