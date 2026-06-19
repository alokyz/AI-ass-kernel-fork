#include "heap.h"
#include "pmm.h"
#include "string.h"

#define BLOCK_MAGIC 0xDEADBEEF
#define ALIGN4(x) (((x) + 3) & ~3)

typedef struct block_header {
    uint32_t magic;
    uint32_t size;
    uint32_t used;
    struct block_header* next;
} block_header_t;

static block_header_t* free_list;
static uint32_t heap_current;

void heap_init(void) {
    free_list = NULL;
    heap_current = HEAP_START;
}

static block_header_t* heap_expand(uint32_t size) {
    uint32_t total = sizeof(block_header_t) + size;
    uint32_t pages = (total + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    uint32_t i;

    for (i = 0; i < pages; i++) {
        uint32_t page = pmm_alloc_page();
        if (!page) return NULL;
        memset((void*)page, 0, PMM_PAGE_SIZE);
    }

    block_header_t* blk = (block_header_t*)heap_current;
    blk->magic = BLOCK_MAGIC;
    blk->size  = pages * PMM_PAGE_SIZE - sizeof(block_header_t);
    blk->used  = 0;
    blk->next  = free_list;
    free_list  = blk;

    heap_current += pages * PMM_PAGE_SIZE;
    return blk;
}

void* kmalloc(uint32_t size) {
    if (size == 0) return NULL;
    size = ALIGN4(size);

    block_header_t* blk = free_list;
    while (blk) {
        if (blk->magic == BLOCK_MAGIC && !blk->used && blk->size >= size) {
            if (blk->size >= size + sizeof(block_header_t) + 16) {
                block_header_t* new_blk = (block_header_t*)((uint8_t*)blk + sizeof(block_header_t) + size);
                new_blk->magic = BLOCK_MAGIC;
                new_blk->size  = blk->size - size - sizeof(block_header_t);
                new_blk->used  = 0;
                new_blk->next  = blk->next;
                blk->next = new_blk;
                blk->size = size;
            }
            blk->used = 1;
            return (void*)((uint8_t*)blk + sizeof(block_header_t));
        }
        blk = blk->next;
    }

    blk = heap_expand(size);
    if (!blk) return NULL;
    blk->used = 1;
    return (void*)((uint8_t*)blk + sizeof(block_header_t));
}

void* kcalloc(uint32_t num, uint32_t size) {
    uint32_t total = num * size;
    void* ptr = kmalloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* krealloc(void* ptr, uint32_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) { kfree(ptr); return NULL; }

    block_header_t* blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    if (blk->magic != BLOCK_MAGIC) return NULL;

    if (blk->size >= size) return ptr;

    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, blk->size);
        kfree(ptr);
    }
    return new_ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    block_header_t* blk = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    if (blk->magic != BLOCK_MAGIC) return;
    blk->used = 0;

    block_header_t* cur = free_list;
    while (cur && cur->next) {
        if (!cur->used && !cur->next->used &&
            (uint8_t*)cur + sizeof(block_header_t) + cur->size == (uint8_t*)cur->next) {
            cur->size += sizeof(block_header_t) + cur->next->size;
            cur->next = cur->next->next;
            continue;
        }
        cur = cur->next;
    }
}
