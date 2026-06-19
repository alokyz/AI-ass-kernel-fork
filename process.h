#ifndef ORANGEX_PROCESS_H
#define ORANGEX_PROCESS_H

#include "types.h"
#include "pmm.h"
#include "isr.h"

#define MAX_PROCESSES 32
#define PROCESS_KERNEL_STACK (PMM_PAGE_SIZE * 2)
#define PROCESS_USER_STACK   (PMM_PAGE_SIZE * 2)

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

typedef struct {
    uint32_t pid;
    char     name[32];
    proc_state_t state;
    uint32_t esp;
    uint32_t ebp;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t wake_tick;
    uint32_t is_user;
} process_t;

void     process_init(void);
uint32_t process_create(const char* name, uint32_t entry, int user);
void     process_exit(void);
void     process_sleep(uint32_t ms);
void     process_block(uint32_t wake_tick);
void     process_unblock(uint32_t pid);
process_t* process_get_current(void);
process_t* process_get(uint32_t pid);
uint32_t process_get_count(void);
void     scheduler_tick(registers_t* r);

extern void switch_context(uint32_t* old_esp, uint32_t new_esp);
extern void switch_to_user_mode(uint32_t eip, uint32_t esp);

#endif
