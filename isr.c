#include "isr.h"
#include "idt.h"
#include "vga.h"
#include "port.h"

static isr_callback_t callbacks[IDT_ENTRIES];

void isr_register_callback(uint32_t n, isr_callback_t cb) {
    if (n < IDT_ENTRIES)
        callbacks[n] = cb;
}

void isr_handler(registers_t* r) {
    if (r->int_no < IDT_ENTRIES && callbacks[r->int_no]) {
        callbacks[r->int_no](r);
        return;
    }
    if (r->int_no < 32) {
        vga_puts_color("\n*** CPU EXCEPTION: ", VGA_COLOR(VGA_WHITE, VGA_RED));
        switch (r->int_no) {
            case 0:  vga_puts("Division By Zero"); break;
            case 6:  vga_puts("Invalid Opcode"); break;
            case 13: vga_puts("General Protection Fault"); break;
            case 14: vga_puts("Page Fault"); break;
            default: vga_puts("Exception "); vga_print_dec(r->int_no); break;
        }
        vga_puts(" ***\n");
        vga_puts("  EIP: "); vga_print_hex(r->eip);
        vga_puts("  ERR: "); vga_print_hex(r->err_code);
        vga_puts("\n  System halted.\n");
        for (;;) __asm__ __volatile__("cli; hlt");
    }
}

void irq_handler(registers_t* r) {
    if (r->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
    if (r->int_no < IDT_ENTRIES && callbacks[r->int_no])
        callbacks[r->int_no](r);
}
