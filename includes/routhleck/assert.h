#ifndef __ROUTHLECK_ASSERT_H
#define __ROUTHLECK_ASSERT_H

#include <libc/stdio.h>
#include <routhleck/tty/tty.h>

#ifdef __ROUTHLECKOS_DEBUG__
#define assert(cond)                                  \
    if (!(cond)) {                                    \
        __assert_fail(#cond, __FILE__, __LINE__);     \
    }
#else
#define assert(cond) //nothing
#endif


void __assert_fail(const char* expr, const char* file, unsigned int line) __attribute__((noinline, noreturn));

#endif