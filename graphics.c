#include "graphics.h"
#include "string.h"
#include "heap.h"
#include "port.h"

extern uint8_t font8x8_basic[256][8];

static gfx_t gfx_state;

static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
static void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val);

void gfx_init(multiboot_info_t* mboot) {
    memset(&gfx_state, 0, sizeof(gfx_t));
    if (!(mboot->flags & MULTIBOOT_FRAMEBUFFER)) return;
    uint32_t fb_addr = *((uint32_t*)((uint8_t*)mboot + 88));
    uint32_t pitch   = *((uint32_t*)((uint8_t*)mboot + 92));
    uint32_t width   = *((uint32_t*)((uint8_t*)mboot + 96));
    uint32_t height  = *((uint32_t*)((uint8_t*)mboot + 100));
    uint8_t  bpp     = *((uint8_t*)((uint8_t*)mboot + 104));
    if (fb_addr == 0 || width == 0 || height == 0) return;
    gfx_state.framebuffer = (uint32_t*)(uint32_t)fb_addr;
    gfx_state.width  = width;
    gfx_state.height = height;
    gfx_state.pitch  = pitch;
    gfx_state.bpp    = bpp;
    gfx_state.size   = pitch * height;
    gfx_state.backbuffer = (uint32_t*)kmalloc(gfx_state.size);
    if (!gfx_state.backbuffer) gfx_state.backbuffer = gfx_state.framebuffer;
    gfx_state.ready = 1;
    gfx_clear(COLOR_BG);
    gfx_flip();
}

int gfx_width(void)  { return gfx_state.ready ? (int)gfx_state.width : 80; }
int gfx_height(void) { return gfx_state.ready ? (int)gfx_state.height : 25; }
gfx_t* gfx_get(void) { return &gfx_state; }

void gfx_init_vbe(void* vbe_data) {
    uint32_t* d = (uint32_t*)vbe_data;
    uint32_t fb_addr  = d[0];
    uint32_t width    = d[2];
    uint32_t height   = d[3];
    uint32_t valid    = d[5];
    memset(&gfx_state, 0, sizeof(gfx_t));
    if (valid != 0x4F5258 || fb_addr == 0 || width == 0 || height == 0) return;

    gfx_state.framebuffer = (uint32_t*)(uint32_t)fb_addr;
    gfx_state.width  = width;
    gfx_state.height = height;
    gfx_state.pitch  = width * 4;
    gfx_state.bpp    = 32;
    gfx_state.size   = gfx_state.pitch * gfx_state.height;
    gfx_state.backbuffer = (uint32_t*)kmalloc(gfx_state.size);
    if (!gfx_state.backbuffer) gfx_state.backbuffer = gfx_state.framebuffer;
    gfx_state.ready = 1;
    gfx_clear(COLOR_BG);
    gfx_flip();
}

int gfx_init_pci(void) {
    uint8_t bus, dev;
    memset(&gfx_state, 0, sizeof(gfx_t));

    for (bus = 0; bus < 255; bus++) {
        for (dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0xFFFF) continue;
            uint32_t class_rev = pci_read(bus, dev, 0, 8);
            uint8_t class_code = (class_rev >> 24) & 0xFF;
            uint8_t subclass   = (class_rev >> 16) & 0xFF;
            if (class_code == 0x03 && subclass == 0x00) {
                uint32_t bar0 = pci_read(bus, dev, 0, 0x10);
                if (bar0 == 0 || (bar0 & 1)) continue;
                uint32_t fb_addr = bar0 & 0xFFFFFFF0;
                pci_write(bus, dev, 0, 0x04, pci_read(bus, dev, 0, 0x04) | 2);
                gfx_state.framebuffer = (uint32_t*)fb_addr;
                gfx_state.width  = 1024;
                gfx_state.height = 768;
                gfx_state.pitch  = 4096;
                gfx_state.bpp    = 32;
                gfx_state.size   = 4096 * 768;
                gfx_state.backbuffer = (uint32_t*)kmalloc(gfx_state.size);
                if (!gfx_state.backbuffer) gfx_state.backbuffer = gfx_state.framebuffer;
                gfx_state.ready = 1;
                gfx_clear(0x1A1B26);
                gfx_flip();
                return 1;
            }
        }
    }
    return 0;
}

static uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    uint32_t addr = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) |
                     ((uint32_t)func << 8) | (off & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val) {
    uint32_t addr = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) |
                     ((uint32_t)func << 8) | (off & 0xFC);
    outl(0xCF8, addr);
    outl(0xCFC, val);
}

void gfx_put_pixel(int x, int y, uint32_t color) {
    if (!gfx_state.ready) return;
    if (x < 0 || y < 0 || x >= (int)gfx_state.width || y >= (int)gfx_state.height) return;
    gfx_state.backbuffer[y * (gfx_state.pitch / 4) + x] = color;
}

uint32_t gfx_get_pixel(int x, int y) {
    if (!gfx_state.ready) return 0;
    if (x < 0 || y < 0 || x >= (int)gfx_state.width || y >= (int)gfx_state.height) return 0;
    return gfx_state.backbuffer[y * (gfx_state.pitch / 4) + x];
}

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    int i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            gfx_put_pixel(i, j, color);
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    int i;
    for (i = x; i < x + w; i++) { gfx_put_pixel(i, y, color); gfx_put_pixel(i, y+h-1, color); }
    for (i = y; i < y + h; i++) { gfx_put_pixel(x, i, color); gfx_put_pixel(x+w-1, i, color); }
}

void gfx_draw_rect_thick(int x, int y, int w, int h, uint32_t color, int t) {
    gfx_fill_rect(x, y, w, t, color);
    gfx_fill_rect(x, y+h-t, w, t, color);
    gfx_fill_rect(x, y, t, h, color);
    gfx_fill_rect(x+w-t, y, t, h, color);
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        gfx_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void gfx_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg) {
    int cx, cy;
    unsigned char c = (unsigned char)ch;
    if (c >= 128) c = '?';
    for (cy = 0; cy < 8; cy++) {
        uint8_t line = font8x8_basic[c][cy];
        for (cx = 0; cx < 8; cx++) {
            if (line & (1 << cx))
                gfx_put_pixel(x + cx, y + cy, fg);
            else if (bg != 0xFFFFFFFF)
                gfx_put_pixel(x + cx, y + cy, bg);
        }
    }
}

void gfx_draw_string(int x, int y, const char* s, uint32_t fg, uint32_t bg) {
    int ox = x;
    while (*s) {
        if (*s == '\n') { x = ox; y += 8; s++; continue; }
        gfx_draw_char(x, y, *s, fg, bg);
        x += 8;
        s++;
    }
}

void gfx_draw_string_large(int x, int y, const char* s, uint32_t fg, uint32_t bg) {
    int ox = x;
    while (*s) {
        if (*s == '\n') { x = ox; y += 16; s++; continue; }
        int cx, cy;
        unsigned char c = (unsigned char)*s;
        if (c >= 128) c = '?';
        for (cy = 0; cy < 8; cy++) {
            uint8_t line = font8x8_basic[c][cy];
            for (cx = 0; cx < 8; cx++) {
                uint32_t px = (line & (1 << cx)) ? fg : bg;
                gfx_put_pixel(x+cx*2, y+cy*2, px);
                gfx_put_pixel(x+cx*2+1, y+cy*2, px);
                gfx_put_pixel(x+cx*2, y+cy*2+1, px);
                gfx_put_pixel(x+cx*2+1, y+cy*2+1, px);
            }
        }
        x += 16;
        s++;
    }
}

void gfx_clear(uint32_t color) {
    if (!gfx_state.ready) return;
    uint32_t i;
    uint32_t total = (gfx_state.pitch / 4) * gfx_state.height;
    for (i = 0; i < total; i++) gfx_state.backbuffer[i] = color;
}

void gfx_flip(void) {
    if (!gfx_state.ready) return;
    if (gfx_state.backbuffer == gfx_state.framebuffer) return;
    memcpy(gfx_state.framebuffer, gfx_state.backbuffer, gfx_state.size);
}

void gfx_draw_gradient(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
    int j;
    for (j = 0; j < h; j++) {
        uint32_t r = ((c1 >> 16) & 0xFF) + (((c2 >> 16) & 0xFF) - ((c1 >> 16) & 0xFF)) * j / h;
        uint32_t g = ((c1 >> 8) & 0xFF) + (((c2 >> 8) & 0xFF) - ((c1 >> 8) & 0xFF)) * j / h;
        uint32_t b = (c1 & 0xFF) + ((c2 & 0xFF) - (c1 & 0xFF)) * j / h;
        uint32_t color = (r << 16) | (g << 8) | b;
        gfx_fill_rect(x, y + j, w, 1, color);
    }
}
