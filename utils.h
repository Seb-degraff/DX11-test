#pragma once


#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)

#define log(...) log_impl(WIDE1(__FILE__), __LINE__, __VA_ARGS__)
void log_impl(const wchar_t* file, int line, LPCWSTR format, ...);



