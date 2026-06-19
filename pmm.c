#include "pmm.h"
#include "multiboot.h"
#include "string.h"

#define MAX_PAGES 65536
#define BITMAP_WORDS (MAX_PAGES / 32)

static uint32_t bitmap[BITMAP_WORDS];
static uint32_t total_pages;
static uint32_t used_pages;

extern uint32_t _kernel_end;

static void pmm_set_bit(uint32_t page) {
    bitmap[page / 32] |= (1U << (page % 32));
}

static void pmm_clear_bit(uint32_t page) {
    bitmap[page / 32] &= ~(1U << (page % 32));
}

static int pmm_test_bit(uint32_t page) {
    return (bitmap[page / 32] >> (page % 32)) & 1;
}

void pmm_init(uint32_t mmap_addr, uint32_t mmap_len) {
    uint32_t i;
    memset(bitmap, 0xFF, sizeof(bitmap));
    total_pages = 0;
    used_pages = 0;

    uint32_t end = (uint32_t)&_kernel_end;
    uint32_t kernel_end_page = (end + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)mmap_addr;
    uint32_t offset = 0;

    while (offset < mmap_len) {
        uint32_t base_lo  = mmap->base_low;
        uint32_t base_hi  = mmap->base_high;
        uint32_t len_lo   = mmap->length_low;
        uint32_t len_hi   = mmap->length_high;
        uint32_t type     = mmap->type;

        (void)base_hi;
        (void)len_hi;

        if (type == 1 && base_lo >= PMM_PAGE_SIZE) {
            uint32_t start_page = base_lo / PMM_PAGE_SIZE;
            uint32_t page_count = len_lo / PMM_PAGE_SIZE;
            uint32_t end_page = start_page + page_count;

            if (end_page > MAX_PAGES)
                end_page = MAX_PAGES;

            for (i = start_page; i < end_page; i++) {
                if (i >= kernel_end_page) {
                    pmm_clear_bit(i);
                    total_pages++;
                }
            }
        }

        offset += mmap->size + sizeof(uint32_t);
        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(uint32_t));
    }
    used_pages = total_pages;
}

void pmm_init_flat(uint32_t mem_start, uint32_t mem_end) {
    uint32_t i;
    memset(bitmap, 0xFF, sizeof(bitmap));
    total_pages = 0;
    used_pages = 0;
    uint32_t start_page = (mem_start + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    uint32_t end_page = mem_end / PMM_PAGE_SIZE;
    if (end_page > MAX_PAGES) end_page = MAX_PAGES;
    for (i = start_page; i < end_page; i++) {
        pmm_clear_bit(i);
        total_pages++;
    }
    used_pages = total_pages;
}

uint32_t pmm_alloc_page(void) {
    uint32_t i, j;
    for (i = 0; i < BITMAP_WORDS; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1U << j))) {
                    uint32_t page = i * 32 + j;
                    pmm_set_bit(page);
                    used_pages++;
                    return page * PMM_PAGE_SIZE;
                }
            }
        }
    }
    return 0;
}

void pmm_free_page(uint32_t addr) {
    uint32_t page = addr / PMM_PAGE_SIZE;
    if (page < MAX_PAGES && pmm_test_bit(page)) {
        pmm_clear_bit(page);
        used_pages--;
    }
}

uint32_t pmm_get_free(void) {
    return total_pages - used_pages;
}

uint32_t pmm_get_total(void) {
    return total_pages;
}
