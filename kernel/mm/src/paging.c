#include "../paging.h"

static uint64_t get_cr3() {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}

static void set_cr3(uint64_t cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (cr3) : "memory");
}

static void invlpg(void* addr) {
    __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

static void get_page_indices(uint64_t vaddr, 
                            uint16_t* pml4_index,
                            uint16_t* pdpt_index,
                            uint16_t* pd_index,
                            uint16_t* pt_index) {
    *pml4_index = (vaddr >> 39) & 0x1FF;
    *pdpt_index = (vaddr >> 30) & 0x1FF;
    *pd_index = (vaddr >> 21) & 0x1FF;
    *pt_index = (vaddr >> 12) & 0x1FF;
}

bool map_page(uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    uint16_t pml4_index, pdpt_index, pd_index, pt_index;
    get_page_indices(vaddr, &pml4_index, &pdpt_index, &pd_index, &pt_index);
    
    uint64_t cr3 = get_cr3() & ~0xFFF;
    page_entry_t* pml4_table = (page_entry_t*)cr3;
    
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        void* pdpt_addr = kmalloc(4096);
        uint64_t pdpt_paddr = (uint64_t)pdpt_addr;
        memset(pdpt_addr, 0, 4096);
        
        pml4_table[pml4_index] = pdpt_paddr | PAGE_PRESENT | PAGE_WRITABLE | flags;
    }
    
    page_entry_t* pdpt_table = (page_entry_t*)((pml4_table[pml4_index] & ~0xFFF));
    
    if (!(pdpt_table[pdpt_index] & PAGE_PRESENT)) {
        void* pd_addr = kmalloc(4096);
        uint64_t pd_paddr = (uint64_t)pd_addr;
        memset(pd_addr, 0, 4096);
        
        pdpt_table[pdpt_index] = pd_paddr | PAGE_PRESENT | PAGE_WRITABLE | flags;
    }
    
    page_entry_t* pd_table = (page_entry_t*)((pdpt_table[pdpt_index] & ~0xFFF));
    
    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        void* pt_addr = kmalloc(4096);
        uint64_t pt_paddr = (uint64_t)pt_addr;
        memset(pt_addr, 0, 4096);
        
        pd_table[pd_index] = pt_paddr | PAGE_PRESENT | PAGE_WRITABLE | flags;
    }
    
    page_entry_t* pt_table = (page_entry_t*)((pd_table[pd_index] & ~0xFFF));
    
    pt_table[pt_index] = (paddr & ~0xFFF) | flags;
    
    invlpg((void*)vaddr);    
    return true;
}

void unmap_page(uint64_t vaddr) {
    uint16_t pml4_index, pdpt_index, pd_index, pt_index;
    get_page_indices(vaddr, &pml4_index, &pdpt_index, &pd_index, &pt_index);
    
    uint64_t cr3 = get_cr3() & ~0xFFF;
    page_entry_t* pml4_table = (page_entry_t*)cr3;
    
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        return;
    }
    
    page_entry_t* pdpt_table = (page_entry_t*)((pml4_table[pml4_index] & ~0xFFF));
    
    if (!(pdpt_table[pdpt_index] & PAGE_PRESENT)) {
        return;
    }
    
    page_entry_t* pd_table = (page_entry_t*)((pdpt_table[pdpt_index] & ~0xFFF));
    
    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        return;
    }
    
    page_entry_t* pt_table = (page_entry_t*)((pd_table[pd_index] & ~0xFFF));
    
    pt_table[pt_index] = 0;
    
    invlpg((void*)vaddr);
}

uint64_t get_physical_address(uint64_t vaddr) {
    uint16_t pml4_index, pdpt_index, pd_index, pt_index;
    get_page_indices(vaddr, &pml4_index, &pdpt_index, &pd_index, &pt_index);
    
    uint64_t cr3 = get_cr3() & ~0xFFF;
    page_entry_t* pml4_table = (page_entry_t*)cr3;
    
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_entry_t* pdpt_table = (page_entry_t*)((pml4_table[pml4_index] & ~0xFFF));
    
    if (!(pdpt_table[pdpt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_entry_t* pd_table = (page_entry_t*)((pdpt_table[pdpt_index] & ~0xFFF));
    
    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    if (pd_table[pd_index] & PAGE_HUGE) {
        return (pd_table[pd_index] & ~0x1FFFFF) + (vaddr & 0x1FFFFF);
    }
    
    page_entry_t* pt_table = (page_entry_t*)((pd_table[pd_index] & ~0xFFF));
    
    if (!(pt_table[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (pt_table[pt_index] & ~0xFFF) + (vaddr & 0xFFF);
}

void paging_init() {
    kprintf("Initializing paging...\n");
    
    uint64_t cr3 = get_cr3();
    kprintf("CR3 = %x\n", cr3);
    
    for (uint64_t addr = HEAP_START; addr < HEAP_START + HEAP_SIZE; addr += 4096) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    uint64_t fs_memory_start = 0x1000000; // 16 MB
    for (uint64_t addr = fs_memory_start; addr < fs_memory_start + 0x400000; addr += 4096) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    kprintf("Paging initialized with filesystem memory region mapped\n");
}