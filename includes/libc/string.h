#ifndef __ROUTHLECK_STRING_H
#define __ROUTHLECK_STRING_H

#include <stddef.h>

int
memcmp(const void*, const void*, size_t);

void*
memcpy(void* __restrict, const void* __restrict, size_t);

void*
memmove(void*, const void*, size_t);

void*
memset(void*, int, size_t);

size_t
strlen(const char* str);

char*
strcpy(char* dest, const char* src);

#endif /* __LUNAIX_STRING_H */
