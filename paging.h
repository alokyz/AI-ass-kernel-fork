#ifndef ORANGEX_PAGING_H
#define ORANGEX_PAGING_H

#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_PRESENT  0x01
#define PAGE_WRITE     0x02
#define PAGE_USER      0x04

typedef struct {
    uint32_t entries[1024];
} __attribute__((aligned(4096))) page_table_t;

typedef struct {
    uint32_t entries[1024];
} __attribute__((aligned(4096))) page_directory_t;

void     paging_init(void);
void     paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void     paging_unmap(uint32_t virt);
uint32_t paging_get_physical(uint32_t virt);
void     paging_switch_directory(page_directory_t* dir);
page_directory_t* paging_create_directory(void);

#endif
