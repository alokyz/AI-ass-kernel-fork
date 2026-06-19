#ifndef ORANGEX_SCHEDULER_H
#define ORANGEX_SCHEDULER_H

#include "types.h"
#include "process.h"

void scheduler_init(void);
void scheduler_tick(registers_t* r);
void scheduler_yield(void);
void scheduler_sleep(uint32_t ms);
void scheduler_reschedule(void);

#endif
