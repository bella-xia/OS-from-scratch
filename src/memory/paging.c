#include "memory/paging.h"
#include "memory/kheap.h"
#include "status.h"

static uint32_t *current_directory = 0;

void paging_load_directory(uint32_t *directory);

uint32_t *palloc_get_page() {
    return (uint32_t *)kzalloc(PAGING_PAGE_SIZE);
}

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags) {
    uint32_t *directory = palloc_get_page();
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t *entry = palloc_get_page();
        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
            entry[j] = (offset++ * PAGING_PAGE_SIZE) | flags;
        }
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk *chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

void paging_switch(uint32_t *directory) {
    paging_load_directory(directory);
    current_directory = directory;
}

uint32_t *paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk) {
    return chunk->directory_entry;
}

bool paging_is_aligned(void *addr) {
    return ((uint32_t)addr % PAGING_PAGE_SIZE == 0);
}

int paging_get_indexes(void *virtual_addr, uint32_t *directory_index_out,
uint32_t *table_index_out) {
    int res = PEACHOS_ALL_OK;
    if (!paging_is_aligned(virtual_addr)) {
        res = -EIVARG;
        goto out;
    }

    *directory_index_out = ((uint32_t) virtual_addr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index_out = ((uint32_t) virtual_addr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);

out:
    return res;

}

int paging_set(uint32_t *directory, void *virtual_addr, uint32_t val) {
    int res = PEACHOS_ALL_OK;
    if (!paging_is_aligned(virtual_addr)) {
        res = -EIVARG;
        goto out;
    }

    uint32_t directory_index = 0, table_index = 0;
    res = paging_get_indexes(virtual_addr, &directory_index, &table_index);
    if (res < 0)
        goto out;
    
    uint32_t entry = directory[directory_index];
    uint32_t *table = (uint32_t *) (entry & 0xfffff000);
    table[table_index] = val;

out:
    return res;
}