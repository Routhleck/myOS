#define __ROUTHLECK_LIBC
#include <libc/stdio.h>
#include <stdarg.h>

#include <routhleck/tty/tty.h>

void
printf(char* fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    __sprintf_internal(buffer, fmt, args);
    va_end(args);

    tty_put_str(buffer);
}