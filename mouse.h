#ifndef ORANGEX_MOUSE_H
#define ORANGEX_MOUSE_H

#include "types.h"
#include "isr.h"

typedef struct {
    int x, y;
    int dx, dy;
    int buttons;
    int scroll;
    int active;
} mouse_state_t;

void mouse_init(void);
void mouse_handler(registers_t* r);
mouse_state_t* mouse_get_state(void);

#endif
