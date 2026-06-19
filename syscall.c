#include "syscall.h"
#include "isr.h"
#include "process.h"
#include "timer.h"
#include "vga.h"
#include "string.h"

extern void syscall_stub(void);

static uint32_t sys_exit(registers_t* r) {
    UNUSED(r);
    process_exit();
    return 0;
}

static uint32_t sys_write(registers_t* r) {
    const char* str = (const char*)r->ebx;
    uint32_t len = r->ecx;
    uint32_t i;
    if (!str) return 0;
    for (i = 0; i < len && str[i]; i++)
        vga_putc(str[i]);
    return i;
}

static uint32_t sys_read(registers_t* r) {
    UNUSED(r);
    return 0;
}

static uint32_t sys_getpid(registers_t* r) {
    UNUSED(r);
    process_t* p = process_get_current();
    return p ? p->pid : 0;
}

static uint32_t sys_sleep(registers_t* r) {
    process_sleep(r->ebx);
    return 0;
}

static uint32_t sys_fork(registers_t* r) {
    UNUSED(r);
    process_t* p = process_get_current();
    if (!p) return 0;
    return process_create(p->name, 0, p->is_user);
}

static uint32_t sys_exec(registers_t* r) {
    UNUSED(r);
    return 0;
}

static uint32_t sys_wait(registers_t* r) {
    UNUSED(r);
    return 0;
}

static uint32_t sys_reboot(registers_t* r) {
    UNUSED(r);
    for (;;) __asm__ __volatile__("cli; hlt");
    return 0;
}

typedef uint32_t (*syscall_fn)(registers_t*);

static syscall_fn syscall_table[SYSCALL_COUNT] = {
    sys_exit, sys_write, sys_read, sys_getpid,
    sys_sleep, sys_fork, sys_exec, sys_wait, sys_reboot
};

static void syscall_handler(registers_t* r) {
    if (r->eax < SYSCALL_COUNT && syscall_table[r->eax]) {
        r->eax = syscall_table[r->eax](r);
    } else {
        r->eax = (uint32_t)-1;
    }
}

void syscall_init(void) {
    isr_register_callback(0x80, syscall_handler);
}
