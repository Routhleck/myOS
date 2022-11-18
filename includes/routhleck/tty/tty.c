#include "tty.h"
#include <stdint.h>

#define TTY_WIDTH 80
#define TTY_HEIGHT 25

vga_attributes *buffer = 0xB8000;

vga_attributes theme_color = VGA_COLOR_BLACK;

uint32_t TTY_COLUMN = 0;
uint16_t TTY_ROW = 0;
void tty_set_theme(vga_attributes fg, vga_attributes bg) {
    theme_color = (bg << 4 | fg) << 8;
}

void tty_put_char(char chr) {
    vga_attributes attr = theme_color |chr;
    *(buffer + TTY_COLUMN + TTY_ROW * TTY_WIDTH) = attr;
}

void tty_put_str(char* str) {

}

void tty_scroll_up() {

}

void tty_clear() {

}