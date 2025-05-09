#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH  0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITEABLE   0b00000010
#define PAGING_IS_PRESENT     0b00000001

#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE (sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE)


struct paging_4gb_chunk {
    uint32_t *directory_entry;
};

uint32_t *palloc_get_page(void);
void paging_switch(uint32_t *directory);
struct paging_4gb_chunk *paging_new_4gb(uint8_t flags);
void enable_paging(void);
int paging_set(uint32_t *directory, void *virtual_addr, uint32_t val);
bool paging_is_aligned(void *addr);

#endif