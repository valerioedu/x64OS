#include "cpu.h"
#include "../../lib/definitions.h"

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)

void setup_paging() {
    uint64_t* pml4 = (uint64_t*)0x100000;
    uint64_t* pdpt = (uint64_t*)0x101000;
    uint64_t* pd = (uint64_t*)0x102000;
    uint64_t* pt = (uint64_t*)0x103000;

    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
        pt[i] = 0;
    }

    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITABLE;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITABLE;
    pd[0] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITABLE;

    for (int i = 0; i < 512; i++) {
        pt[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /*__asm__ volatile (
        ".code32\n"
        "mov %%cr4, %%eax\n"
        "or $0x20, %%eax\n"
        "mov %%eax, %%cr4\n"
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0"
        : : "r" (pml4) : "eax"
    );*/
}