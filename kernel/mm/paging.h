#ifndef PAGING_H
#define PAGING_H

#include "../../lib/definitions.h"
#include "heap.h"

#define PAGE_SIZE 4096
#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITABLE   (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_WRITETHROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED   (1ULL << 5)
#define PAGE_DIRTY      (1ULL << 6)
#define PAGE_HUGE       (1ULL << 7)
#define PAGE_GLOBAL     (1ULL << 8)
#define PAGE_NX         (1ULL << 63)

#define MEMORY_SIZE     (256 * 1024 * 128) // 8MB of memory
#define PAGE_COUNT      (MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE     (PAGE_COUNT / 8)

typedef uint64_t page_entry_t;

void paging_init(void);

// Map a virtual address to a physical address
bool map_page(uint64_t vaddr, uint64_t paddr, uint64_t flags);

// Unmap a virtual address
void unmap_page(uint64_t vaddr);

// Get the physical address for a virtual address
uint64_t get_physical_address(uint64_t vaddr);

#endif