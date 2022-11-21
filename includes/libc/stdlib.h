#ifndef __ROUTHLECK_STDLIB_H
#define __ROUTHLECK_STDLIB_H

#ifdef __ROUTHLECK_LIBC
char* __uitoa_internal (unsigned int value, char* str, int base, unsigned int* size);
char* __itoa_internal (int value, char* str, int base, unsigned int* size);
#endif
char* itoa (int value, char* str, int base);

#endif