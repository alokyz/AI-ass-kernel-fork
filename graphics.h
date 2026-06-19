#ifndef ORANGEX_GRAPHICS_H
#define ORANGEX_GRAPHICS_H

#include "types.h"
#include "multiboot.h"

#define COLOR(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define COLOR_WHITE   0xFFFFFF
#define COLOR_BLACK   0x000000
#define COLOR_RED     0xFF4444
#define COLOR_GREEN   0x44FF44
#define COLOR_BLUE    0x4488FF
#define COLOR_YELLOW  0xFFFF44
#define COLOR_CYAN    0x44FFFF
#define COLOR_ORANGE  0xFF8844
#define COLOR_GRAY    0x888888
#define COLOR_LTGRAY  0xCCCCCC
#define COLOR_DKGRAY  0x444444
#define COLOR_BG      0x1A1B26
#define COLOR_TASKBAR 0x24283B
#define COLOR_TITLE   0x7AA2F7
#define COLOR_BTN     0xBB9AF7

typedef struct {
    uint32_t* framebuffer;
    uint32_t* backbuffer;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint32_t  bpp;
    uint32_t  size;
    int       ready;
} gfx_t;

void gfx_init(multiboot_info_t* mboot);
void gfx_init_vbe(void* vbe_data);
int  gfx_init_pci(void);
void gfx_put_pixel(int x, int y, uint32_t color);
uint32_t gfx_get_pixel(int x, int y);
void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect_thick(int x, int y, int w, int h, uint32_t color, int t);
void gfx_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void gfx_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_draw_string(int x, int y, const char* s, uint32_t fg, uint32_t bg);
void gfx_draw_string_large(int x, int y, const char* s, uint32_t fg, uint32_t bg);
void gfx_clear(uint32_t color);
void gfx_flip(void);
void gfx_draw_gradient(int x, int y, int w, int h, uint32_t c1, uint32_t c2);
int  gfx_width(void);
int  gfx_height(void);
gfx_t* gfx_get(void);

#endif
