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
    .global start_
    .type start_, @function
    start_:
        movl $stack_top, %esp
        /*
            TODO:
                kernel init
        */
        call _kernel_init

        subl $0x6, %esp
        movl $_gdt, 2(%esp)
        movw _gdt_limit, %ax
        movw %ax, (%esp)
        lgdt (%esp)
        addl $0x6, %esp

        movw $0x10, %cx
        movw %cx, %es
        movw %cx, %ds
        movw %cx, %fs
        movw %cx, %gs
        movw %cx, %ss

        pushw $0x08
        pushl $_after_gdt
        retf

    _after_gdt:
        pushl %ebx
        call _kernel_main

        cli
    j_:
        hlt
        jmp j_