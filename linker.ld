ENTRY(start_)

SECTIONS {
    . = 0x100000;
    
    .text BLOCK(4K): {
        * (.multiboot)
        *(.text)
    }

    .bss BLOCK(4K): {
        * (COMMON)
        * (.bss)
    }

    .data BLOCK(4K) : {
        * (.data)
    }

    .rodata BLOCK(4K) : {
        * (.rodata)
    }
}