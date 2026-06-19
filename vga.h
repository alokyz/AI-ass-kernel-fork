#ifndef ORANGEX_VGA_H
#define ORANGEX_VGA_H

#include "types.h"

#define VGA_MEMORY  0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

enum vga_color {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN, VGA_LIGHT_CYAN,
    VGA_LIGHT_RED, VGA_LIGHT_MAGENTA, VGA_YELLOW, VGA_WHITE
};

#define VGA_COLOR(fg, bg) ((uint8_t)(((bg) << 4) | ((fg) & 0x0F)))

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char* s);
void vga_puts_color(const char* s, uint8_t color);
void vga_put_char_color(char c, uint8_t color);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_set_cursor(int row, int col);
void vga_enable_cursor(uint8_t start, uint8_t end);
void vga_disable_cursor(void);
void vga_print_hex(uint32_t v);
void vga_print_dec(uint32_t v);

#endif
