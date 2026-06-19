#ifndef ORANGEX_HEAP_H
#define ORANGEX_HEAP_H

#include "types.h"

#define HEAP_START 0x01000000
#define HEAP_SIZE  0x01000000

void  heap_init(void);
void* kmalloc(uint32_t size);
void* kcalloc(uint32_t num, uint32_t size);
void* krealloc(void* ptr, uint32_t size);
void  kfree(void* ptr);

#endif
