#include "vga.h"
#include "port.h"

static uint16_t* vga_buf;
static int cur_row;
static int cur_col;
static uint8_t cur_color;

static void vga_update_hw(void) {
    uint16_t pos = (uint16_t)(cur_row * VGA_WIDTH + cur_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void vga_scroll(void) {
    int i;
    for (i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        vga_buf[i] = vga_buf[i + VGA_WIDTH];
    for (i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
        vga_buf[i] = vga_entry(' ', cur_color);
    cur_row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    vga_buf = (uint16_t*)VGA_MEMORY;
    cur_color = VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK);
    vga_enable_cursor(13, 15);
    vga_clear();
}

void vga_clear(void) {
    int i;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_buf[i] = vga_entry(' ', cur_color);
    cur_row = 0;
    cur_col = 0;
    vga_update_hw();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    cur_color = VGA_COLOR(fg, bg);
}

void vga_putc(char c) {
    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\r') {
        cur_col = 0;
    } else if (c == '\t') {
        cur_col = (cur_col + 8) & ~7;
    } else if (c == '\b') {
        if (cur_col > 0) {
            cur_col--;
            vga_buf[cur_row * VGA_WIDTH + cur_col] = vga_entry(' ', cur_color);
        }
    } else {
        vga_buf[cur_row * VGA_WIDTH + cur_col] = vga_entry(c, cur_color);
        cur_col++;
    }
    if (cur_col >= VGA_WIDTH) { cur_col = 0; cur_row++; }
    if (cur_row >= VGA_HEIGHT) vga_scroll();
    vga_update_hw();
}

void vga_put_char_color(char c, uint8_t color) {
    uint8_t prev = cur_color;
    cur_color = color;
    vga_putc(c);
    cur_color = prev;
}

void vga_puts(const char* s) {
    while (*s) vga_putc(*s++);
}

void vga_puts_color(const char* s, uint8_t color) {
    uint8_t prev = cur_color;
    cur_color = color;
    while (*s) vga_putc(*s++);
    cur_color = prev;
}

void vga_set_cursor(int row, int col) {
    cur_row = row;
    cur_col = col;
    vga_update_hw();
}

void vga_enable_cursor(uint8_t start, uint8_t end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_print_hex(uint32_t v) {
    char buf[11];
    int i;
    buf[0] = '0'; buf[1] = 'x';
    for (i = 9; i >= 2; i--) {
        uint8_t n = (uint8_t)(v & 0x0F);
        buf[i] = (n < 10) ? (char)('0' + n) : (char)('a' + n - 10);
        v >>= 4;
    }
    buf[10] = '\0';
    vga_puts(buf);
}

void vga_print_dec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (v == 0) { vga_putc('0'); return; }
    while (v > 0) { buf[i--] = (char)('0' + (v % 10)); v /= 10; }
    vga_puts(&buf[i + 1]);
}
