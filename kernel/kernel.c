#include <routhleck/tty/tty.h>
#include <routhleck/arch/gdt.h>

void _kernel_init() {
    // TODO
}

void _kernel_main(void* info_table) {
    tty_set_theme(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    tty_put_str("Hello, world!\n");
}