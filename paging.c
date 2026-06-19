#include "paging.h"
#include "pmm.h"
#include "heap.h"
#include "string.h"

static page_directory_t* kernel_dir;

static page_table_t* get_table(uint32_t dir_idx) {
    if (kernel_dir->entries[dir_idx] & PAGE_PRESENT) {
        return (page_table_t*)(kernel_dir->entries[dir_idx] & 0xFFFFF000);
    }
    uint32_t page = pmm_alloc_page();
    if (!page) return NULL;
    memset((void*)page, 0, PAGE_SIZE);
    kernel_dir->entries[dir_idx] = page | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    return (page_table_t*)page;
}

void paging_init(void) {
    uint32_t i, page = pmm_alloc_page();
    kernel_dir = (page_directory_t*)page;
    memset(kernel_dir, 0, PAGE_SIZE);

    for (i = 0; i < 1024; i++) {
        kernel_dir->entries[i] = 0 | PAGE_PRESENT | PAGE_WRITE;
    }

    for (i = 0; i < 1024; i++) {
        page_table_t* table = get_table(i);
        if (!table) continue;
        uint32_t j;
        for (j = 0; j < 1024; j++) {
            uint32_t phys = i * 0x400000 + j * PAGE_SIZE;
            if (phys < 0x1000000) {
                table->entries[j] = phys | PAGE_PRESENT | PAGE_WRITE;
            } else {
                table->entries[j] = 0;
            }
        }
    }

    paging_switch_directory(kernel_dir);

    uint32_t cr0;
    __asm__ __volatile__("mov %0, %%cr0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %%cr0, %0" : : "r"(cr0));
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t dir_idx = virt / 0x400000;
    uint32_t table_idx = (virt / PAGE_SIZE) % 1024;
    page_table_t* table = get_table(dir_idx);
    if (!table) return;
    table->entries[table_idx] = (phys & 0xFFFFF000) | (flags & 0x7) | PAGE_PRESENT;
}

void paging_unmap(uint32_t virt) {
    uint32_t dir_idx = virt / 0x400000;
    uint32_t table_idx = (virt / PAGE_SIZE) % 1024;
    if (!(kernel_dir->entries[dir_idx] & PAGE_PRESENT)) return;
    page_table_t* table = (page_table_t*)(kernel_dir->entries[dir_idx] & 0xFFFFF000);
    table->entries[table_idx] = 0;
    __asm__ __volatile__("invlpg (%0)" : : "r"(virt) : "memory");
}

uint32_t paging_get_physical(uint32_t virt) {
    uint32_t dir_idx = virt / 0x400000;
    uint32_t table_idx = (virt / PAGE_SIZE) % 1024;
    if (!(kernel_dir->entries[dir_idx] & PAGE_PRESENT)) return 0;
    page_table_t* table = (page_table_t*)(kernel_dir->entries[dir_idx] & 0xFFFFF000);
    if (!(table->entries[table_idx] & PAGE_PRESENT)) return 0;
    return table->entries[table_idx] & 0xFFFFF000;
}

void paging_switch_directory(page_directory_t* dir) {
    __asm__ __volatile__("mov %0, %%cr3" : : "r"((uint32_t)dir));
}

page_directory_t* paging_create_directory(void) {
    uint32_t page = pmm_alloc_page();
    if (!page) return NULL;
    page_directory_t* dir = (page_directory_t*)page;
    memset(dir, 0, PAGE_SIZE);
    uint32_t i;
    for (i = 0; i < 768; i++) {
        dir->entries[i] = kernel_dir->entries[i];
    }
    return dir;
}
