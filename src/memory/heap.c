#include "kernel.h"
#include "memory/heap.h"
#include "memory/memory.h"
#include "status.h"
#include <stdbool.h>

static bool heap_validate_alignment(void *ptr) {
    return ((uint32_t)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

static int heap_validate_table(void *start, void *end, struct heap_table *tab) {
    int res = 0;
    size_t table_size = (size_t) (end - start);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if (tab->total != total_blocks) {
        res = -EIVARG;
        goto out;
    }

out:
    return res;
}

int heap_create(struct heap *heap, void *start, void *end, struct heap_table *table) {
    int res = 0;
    if ((!heap_validate_alignment(start) || !heap_validate_alignment(end))) {
        res = -EIVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = start;
    heap->table = table;

    res = heap_validate_table(start, end, table);
    if (!res)
        goto out;
    
    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static uint32_t heap_align_value_to_upper(uint32_t val) {
    if (val % PEACHOS_HEAP_BLOCK_SIZE == 0)
        return val;
    
    val -= val % (PEACHOS_HEAP_BLOCK_SIZE);
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry) {
    return entry & 0x0f;
}

int heap_get_start_block(struct heap* heap, uint32_t block) {
    struct heap_table *table = heap->table;
    int bc = 0, bs = -1;

    for (size_t i = 0; i < table->total; i++) {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            print("block not free. continue searching ...\n");
            bc = 0;
            bs = -1;
            continue;
        }
        print("found free block\n");

        if (bs == -1) 
            bs = i;
        
        if (++bc == block) {
            print("found sufficient block");
            break;
        }
    }
    return (bs == -1) ? -ENOMEM : bs;
}
 
void *heap_block_to_address(struct heap *heap, uint32_t block) {
    return heap->saddr + block * PEACHOS_HEAP_BLOCK_SIZE;
}

int heap_address_to_block(struct heap *heap, void *address) {
    return ((int) (address - heap->saddr)) / PEACHOS_HEAP_BLOCK_SIZE;
}

void heap_mark_block_taken(struct heap *heap, uint32_t start_block, int block) {
    int end_block = start_block + block - 1;
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (block > 1) 
        entry |= HEAP_BLOCK_HAS_NEXT;
    
    for (int i = start_block; i <= end_block; i++) {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i < end_block - 1)
            entry |= HEAP_BLOCK_HAS_NEXT;

    }
}

void heap_mark_blocks_free(struct heap *heap, int starting_block) {
    struct heap_table *table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT))
            break; 
    }
}

void *heap_malloc_blocks(struct heap *heap, uint32_t block) {
    void *address = 0;
    int start_block = heap_get_start_block(heap, block);
    
    if (start_block < 0) 
        goto out;
    
    address = heap_block_to_address(heap, start_block);
    heap_mark_block_taken(heap, start_block, block);

out:
    return address;
}


void *heap_malloc(struct heap *heap, size_t size) {
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap *heap, void *ptr) {
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}