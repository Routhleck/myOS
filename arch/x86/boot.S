#include "multiboot.h"

.section .multiboot
    .long MB_MAGIC
    .long MB_ALIGNED_4K_MEM_MAP
    .long CHECKSUM(MB_ALIGNED_4K_MEM_MAP)

.section .bss
    /* the stack must be aligned at 16 bytes boundary */
    .align 16
    stack_bottom:
    .skip 16384, 0
    stack_top:

.section .text
    start_:
        movl $stack_top, %esp
        /*
            TODO:
                kernel init
        */
        call _kernel_init

        pushl %ebx
        call _kernel_main

        cli
    j_:
        hlt
        jmp j_