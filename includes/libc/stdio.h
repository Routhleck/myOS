#ifndef __ROUTHLECK_STDIO_H
#define __ROUTHLECK_STDIO_H
#include <stdarg.h>

#ifdef __ROUTHLECK_LIBC
void __sprintf_internal(char* buffer, char* fmt, va_list args);
#endif

void sprintf(char* buffer, char* fmt, ...);
void printf(char* fmt, ...);

#endif