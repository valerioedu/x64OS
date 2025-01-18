#include "kernel.h"

void kernel_main() {
    vga_init();
    kprint("Hello, World!\n\tx64OS\n");
    heap_init();
    void* ptr = kmalloc(1024);
    if (!ptr) kprint("Memory allocation failed\n");
    if (ptr) {
        kprint("Memory allocation successful\n");
        kfree(ptr);
        kprint("Memory freed\n");
    }
    idt_init();
    kprint("Interrupts enabled\n");
    for (;;) asm volatile ("hlt");
}