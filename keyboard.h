#ifndef ORANGEX_KEYBOARD_H
#define ORANGEX_KEYBOARD_H

#include "types.h"

#define KFIFO_SIZE 512

typedef struct {
    char     buffer[KFIFO_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} kfifo_t;

void keyboard_init(void);
char keyboard_getchar(void);
char keyboard_try_getchar(void);
int  keyboard_has_data(void);
void keyboard_wait_line(char* buf, uint32_t max_len);

#endif
