#include "mouse.h"
#include "isr.h"
#include "port.h"

static mouse_state_t mouse;
static uint8_t mouse_cycle;
static uint8_t mouse_byte[3];

static void mouse_wait(uint8_t a) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (!(inb(0x64) & a)) return;
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(0x02);
    outb(0x64, 0xD4);
    mouse_wait(0x02);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0x01);
    return inb(0x60);
}

void mouse_init(void) {
    uint8_t status;
    mouse.x = 0;
    mouse.y = 0;
    mouse.dx = 0;
    mouse.dy = 0;
    mouse.buttons = 0;
    mouse.scroll = 0;
    mouse.active = 0;
    mouse_cycle = 0;

    mouse_wait(0x02);
    outb(0x64, 0xA8);
    mouse_wait(0x02);
    outb(0x64, 0x20);
    mouse_wait(0x01);
    status = inb(0x60) | 2;
    mouse_wait(0x02);
    outb(0x64, 0x60);
    mouse_wait(0x02);
    outb(0x60, status);

    mouse_write(0xFF);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();

    mouse.x = 512;
    mouse.y = 384;
    mouse.active = 1;

    isr_register_callback(44, mouse_handler);

    uint8_t mask = inb(0xA1);
    mask &= ~(1 << 4);
    outb(0xA1, mask);
}

void mouse_handler(registers_t* r) {
    UNUSED(r);
    uint8_t data = inb(0x60);
    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = data;
            if (data & 0x08) mouse_cycle = 1;
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse.dx = mouse_byte[1];
            mouse.dy = -mouse_byte[2];
            mouse.buttons = mouse_byte[0] & 0x07;
            if (mouse_byte[0] & 0x10) mouse.dx |= 0xFFFFFF00;
            if (mouse_byte[0] & 0x20) mouse.dy |= 0xFFFFFF00;
            mouse.x += mouse.dx;
            mouse.y += mouse.dy;
            if (mouse.x < 0) mouse.x = 0;
            if (mouse.y < 0) mouse.y = 0;
            mouse_cycle = 0;
            break;
    }
}

mouse_state_t* mouse_get_state(void) {
    mouse.dx = 0;
    mouse.dy = 0;
    return &mouse;
}
