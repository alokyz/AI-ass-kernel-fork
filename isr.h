#ifndef ORANGEX_ISR_H
#define ORANGEX_ISR_H

#include "types.h"

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_callback_t)(registers_t*);

void isr_handler(registers_t* regs);
void irq_handler(registers_t* regs);
void isr_register_callback(uint32_t int_no, isr_callback_t callback);

#endif
