#ifndef ORANGEX_TIMER_H
#define ORANGEX_TIMER_H

#include "types.h"

void     timer_init(uint32_t freq);
uint32_t timer_ticks(void);
void     timer_sleep(uint32_t ms);

#endif
