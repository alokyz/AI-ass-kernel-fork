#include "process.h"
#include "pmm.h"
#include "heap.h"
#include "timer.h"
#include "isr.h"
#include "string.h"

static process_t processes[MAX_PROCESSES];
static process_t* current_proc;
static uint32_t next_pid;

extern uint32_t _kernel_end;

process_t* process_get_current(void) {
    return current_proc;
}

process_t* process_get(uint32_t pid) {
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != PROC_TERMINATED)
            return &processes[i];
    }
    return NULL;
}

uint32_t process_get_count(void) {
    uint32_t i, count = 0;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROC_TERMINATED)
            count++;
    }
    return count;
}

void process_init(void) {
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = PROC_TERMINATED;
        processes[i].pid = 0;
    }
    next_pid = 1;
    current_proc = NULL;
}

static uint32_t create_kernel_stack(void) {
    uint32_t pages = PROCESS_KERNEL_STACK / PMM_PAGE_SIZE;
    uint32_t i;
    uint32_t base = (uint32_t)&_kernel_end;
    base = (base + 0xFFFFF000) & 0xFFFFF000;

    for (i = 0; i < pages; i++) {
        uint32_t page = pmm_alloc_page();
        if (!page) return 0;
        memset((void*)page, 0, PMM_PAGE_SIZE);
    }
    return base + pages * PMM_PAGE_SIZE;
}

static int find_slot(void) {
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_TERMINATED)
            return (int)i;
    }
    return -1;
}

uint32_t process_create(const char* name, uint32_t entry, int user) {
    int slot = find_slot();
    if (slot < 0) return 0;

    process_t* proc = &processes[slot];
    uint32_t i;
    for (i = 0; i < sizeof(process_t); i++)
        ((uint8_t*)proc)[i] = 0;

    uint32_t kstack = create_kernel_stack();
    if (!kstack) return 0;

    uint32_t ustack = 0;

    if (user) {
        ustack = (uint32_t)kmalloc(PROCESS_USER_STACK);
        if (!ustack) {
            pmm_free_page(kstack - PROCESS_KERNEL_STACK);
            return 0;
        }
        memset((void*)ustack, 0, PROCESS_USER_STACK);
        ustack += PROCESS_USER_STACK;
    }

    uint32_t* sp = (uint32_t*)kstack;

    if (user) {
        *(--sp) = 0x23;
        *(--sp) = ustack;
        *(--sp) = 0x202;
        *(--sp) = 0x1B;
        *(--sp) = entry;
    } else {
        *(--sp) = 0x202;
        *(--sp) = 0x08;
        *(--sp) = entry;
    }

    *(--sp) = 0;   /* eax */
    *(--sp) = 0;   /* ecx */
    *(--sp) = 0;   /* edx */
    *(--sp) = 0;   /* ebx */
    uint32_t esp_val = (uint32_t)sp;
    *(--sp) = esp_val;  /* esp */
    *(--sp) = 0;   /* ebp */
    *(--sp) = 0;   /* esi */
    *(--sp) = 0;   /* edi */
    *(--sp) = user ? 0x23 : 0x10;

    proc->pid = next_pid++;
    proc->state = PROC_READY;
    proc->esp = (uint32_t)sp;
    proc->ebp = 0;
    proc->kernel_stack = kstack - PROCESS_KERNEL_STACK;
    proc->user_stack = ustack;
    proc->wake_tick = 0;
    proc->is_user = user;

    for (i = 0; name[i] && i < 31; i++)
        proc->name[i] = name[i];
    proc->name[i] = '\0';

    return proc->pid;
}

void process_exit(void) {
    if (!current_proc) return;
    current_proc->state = PROC_TERMINATED;
    if (current_proc->user_stack) {
        kfree((void*)(current_proc->user_stack - PROCESS_USER_STACK));
    }
    current_proc = NULL;
}

void process_sleep(uint32_t ms) {
    if (!current_proc) return;
    current_proc->wake_tick = timer_ticks() + ms;
    current_proc->state = PROC_BLOCKED;
}

void process_block(uint32_t wake_tick) {
    if (!current_proc) return;
    current_proc->wake_tick = wake_tick;
    current_proc->state = PROC_BLOCKED;
}

void process_unblock(uint32_t pid) {
    process_t* p = process_get(pid);
    if (p && p->state == PROC_BLOCKED) {
        p->state = PROC_READY;
    }
}
