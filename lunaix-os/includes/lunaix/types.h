#ifndef __LUNAIX_TYPES_H
#define __LUNAIX_TYPES_H

#include <stdint.h>

#define PEXITTERM 0x100
#define PEXITSTOP 0x200
#define PEXITSIG 0x400

#define PEXITNUM(flag, code) (flag | (code & 0xff))

#define WNOHANG 1
#define WUNTRACED 2
#define WEXITSTATUS(wstatus) ((wstatus & 0xff))
#define WIFSTOPPED(wstatus) ((wstatus & PEXITSTOP))
#define WIFEXITED(wstatus)                                                     \
    ((wstatus & PEXITTERM) && ((char)WEXITSTATUS(wstatus) >= 0))

#define WIFSIGNALED(wstatus) ((wstatus & PEXITSIG))
// TODO: WTERMSIG

typedef int32_t pid_t;

#endif /* __LUNAIX_TYPES_H */
