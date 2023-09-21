#include "utils.h"
#include "framework.h"

#include <stdio.h>
#include <vadefs.h>

void log_impl(const wchar_t* file, int line, LPCWSTR format, ...)
{
    WCHAR buffer[2048];
    WCHAR buffer2[2048];
    va_list arg;
    va_start(arg, line);
    _vsnwprintf_s(buffer, sizeof(buffer), format, arg);
    _snwprintf_s(buffer2, sizeof(buffer2), L"%s %s(%i)\n", buffer, file, line);
    va_end(arg);

    OutputDebugString(buffer2);
}
