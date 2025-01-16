#include "../heap.h"

MemBlock* heap_start = NULL;
MemBlock* heap_end = NULL;

void heap_init() {
    heap_start = (MemBlock*)HEAP_START;
    heap_start->size = HEAP_SIZE;
    heap_start->free = 1;
    heap_start->next = NULL;
    heap_end = heap_start;
}

void split_block(MemBlock* block, size_t size) {
    if (block->size >= size + sizeof(MemBlock) + 16) {
        MemBlock* new_block = (MemBlock*)((char*)block + sizeof(MemBlock) + size);
        new_block->size = block->size - size - sizeof(MemBlock);
        new_block->free = 1;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

MemBlock* find_fit(size_t size) {
    MemBlock* current = heap_start;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void merge_free_blocks() {
    MemBlock* current = heap_start;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(MemBlock);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void* kmalloc(size_t size) {
    size = ALIGN(size);
    MemBlock* block = find_fit(size);
    if (!block) return NULL;
    block->free = 0;
    split_block(block, size);
    return (void*)((char*)block + sizeof(MemBlock));
}

void kfree(void* ptr) {
    if (!ptr) return;
    MemBlock* block = (MemBlock*)((char*)ptr - sizeof(MemBlock));
    block->free = 1;
    merge_free_blocks();
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    MemBlock* block = (MemBlock*)((char*)ptr - sizeof(MemBlock));
    if (block->size >= size) return ptr;
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}