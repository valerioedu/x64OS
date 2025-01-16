#ifndef HEAP_H
#define HEAP_H

#include "../../lib/definitions.h"

#define HEAP_START 0x100000
#define HEAP_SIZE 0x100000
#define ALIGN(size) (((size) + 15) & ~15)

typedef struct MemBlock {
    size_t size;
    int free;
    struct MemBlock* next;
} MemBlock;

void heap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);

#endif