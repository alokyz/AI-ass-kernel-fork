#include "timer.h"
#include "isr.h"
#include "port.h"
#include "process.h"

static volatile uint32_t ticks_count;

static void timer_callback(registers_t* r) {
    ticks_count++;
    scheduler_tick(r);
}

uint32_t timer_ticks(void) {
    return ticks_count;
}

void timer_sleep(uint32_t ms) {
    uint32_t start = ticks_count;
    while (ticks_count - start < ms)
        __asm__ __volatile__("hlt");
}

void timer_init(uint32_t freq) {
    uint32_t divisor = 1193182 / freq;
    uint8_t  lo = (uint8_t)(divisor & 0xFF);
    uint8_t  hi = (uint8_t)((divisor >> 8) & 0xFF);

    ticks_count = 0;

    outb(0x43, 0x36);
    outb(0x40, lo);
    outb(0x40, hi);

    isr_register_callback(32, timer_callback);

    uint8_t mask = inb(0x21);
    mask &= ~(1 << 0);
    outb(0x21, mask);
}
