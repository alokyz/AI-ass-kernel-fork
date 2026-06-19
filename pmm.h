#ifndef ORANGEX_PMM_H
#define ORANGEX_PMM_H

#include "types.h"

#define PMM_PAGE_SIZE 4096

void     pmm_init(uint32_t mmap_addr, uint32_t mmap_len);
void     pmm_init_flat(uint32_t mem_start, uint32_t mem_end);
uint32_t pmm_alloc_page(void);
void     pmm_free_page(uint32_t addr);
uint32_t pmm_get_free(void);
uint32_t pmm_get_total(void);

#endif
