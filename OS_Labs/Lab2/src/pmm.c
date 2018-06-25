#include <os.h>
#include "mylibc.h"

extern _Area _heap;

static void pmm_init();
static void* pmm_alloc(size_t);
static void pmm_free(void*);

MOD_DEF(pmm){
    .init = pmm_init,
    .alloc = pmm_alloc,
    .free = pmm_free,
};

static void* sbrk(size_t size) {
    char* brkp = (char*)_heap.start;
    char* endp = (char*)_heap.end;
    if (size == 0)
        return (void*)brkp;
    void* free = (void*)brkp;
    brkp += size;
    if (brkp >= endp)
        return NULL;
    _heap.start = brkp;
    return free;
}

typedef struct free_block {
    size_t size;
    struct free_block* next;
} free_block;

static free_block free_block_list_head = {0, NULL};

static void pmm_init() {
    // nothing
}

static void* pmm_alloc(size_t size) {
    if (!size)
        return NULL;
    size = (size + sizeof(free_block));
    uint32_t ksize = 0x1;
    while (size) {
        size >>= 1;
        ksize <<= 1;
    }
    size = ksize;
    free_block* block = free_block_list_head.next;
    free_block** head = &(free_block_list_head.next);
    while (block != NULL) {
        if (block->size >= size) {
            *head = block->next;
            return ((char*)block) + sizeof(free_block);
        }
        head = &(block->next);
        block = block->next;
    }
    block = (free_block*)sbrk(size);
    block->size = size;
    return ((char*)block) + sizeof(free_block);
}

static void pmm_free(void* ptr) {
    if (!ptr)
        return;
    free_block* block = (free_block*)((char*)ptr) - sizeof(free_block);
    block->next = free_block_list_head.next;
    free_block_list_head.next = block;
}