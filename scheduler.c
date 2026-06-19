#include "scheduler.h"
#include "timer.h"
#include "string.h"

static uint32_t quantum = 10;

void scheduler_init(void) {
    quantum = 10;
}

void scheduler_tick(registers_t* r) {
    if (!r) return;
    process_t* cur = process_get_current();
    if (!cur) return;

    if (cur->state == PROC_RUNNING) {
        cur->esp = r->esp;
        cur->ebp = r->ebp;
    }

    uint32_t i;
    uint32_t now = timer_ticks();
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_t* p = process_get(i);
        if (p && p->state == PROC_BLOCKED && p->wake_tick != 0 && now >= p->wake_tick) {
            p->state = PROC_READY;
            p->wake_tick = 0;
        }
    }

    int next = -1;
    uint32_t start = cur ? (uint32_t)(cur - (process_t*)0) + 1 : 0;
    for (i = 0; i < MAX_PROCESSES; i++) {
        uint32_t idx = (start + i) % MAX_PROCESSES;
        process_t* p = process_get(idx);
        if (p && p->state == PROC_READY) { next = (int)idx; break; }
    }

    if (next >= 0) {
        if (cur && cur->state == PROC_RUNNING) cur->state = PROC_READY;
        process_t* np = process_get(next);
        if (np) {
            np->state = PROC_RUNNING;
            r->esp = np->esp;
            r->ebp = np->ebp;
        }
    }
}

void scheduler_yield(void) {
    process_t* cur = process_get_current();
    if (cur) {
        uint32_t i;
        uint32_t start = (uint32_t)(cur - (process_t*)0) + 1;
        for (i = 0; i < MAX_PROCESSES; i++) {
            uint32_t idx = (start + i) % MAX_PROCESSES;
            process_t* p = process_get(idx);
            if (p && p->state == PROC_READY) {
                cur->state = PROC_READY;
                p->state = PROC_RUNNING;
                break;
            }
        }
    }
}

void scheduler_sleep(uint32_t ms) {
    process_t* cur = process_get_current();
    if (cur) {
        cur->wake_tick = timer_ticks() + ms;
        cur->state = PROC_BLOCKED;
    }
    __asm__ __volatile__("sti; hlt");
}

void scheduler_reschedule(void) {
    uint32_t i;
    uint32_t now = timer_ticks();
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_t* p = process_get(i);
        if (p && p->state == PROC_BLOCKED && p->wake_tick != 0 && now >= p->wake_tick) {
            p->state = PROC_READY;
            p->wake_tick = 0;
        }
    }
}
