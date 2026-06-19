#include "gdt.h"
#include "port.h"
#include "string.h"

static struct gdt_entry gdt[6];
static struct gdt_ptr   gp;
static struct tss_entry tss;

extern void gdt_flush(uint32_t);
extern void tss_flush(void);

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity  = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = tss_base + sizeof(struct tss_entry);

    gdt_set_gate(5, tss_base, tss_limit, 0xE9, 0x00);

    memset(&tss, 0, sizeof(struct tss_entry));
    tss.ss0  = 0x10;
    tss.esp0 = 0;
    tss.cs   = 0x0B;
    tss.ss   = 0x13;
    tss.ds   = 0x13;
    tss.es   = 0x13;
    tss.fs   = 0x13;
    tss.gs   = 0x13;

    gdt_flush((uint32_t)&gp);
    tss_flush();
}

void gdt_set_tss(uint32_t kernel_esp) {
    tss.esp0 = kernel_esp;
}
