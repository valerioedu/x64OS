#include "kernel.h"
#include "../drivers/keyboard/keyboard.h"

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
    keyboard_init();
    idt_init();
    kprint("Interrupts enabled\n");
    keyboard_init();
    char* str = read();
    kprint(str);
    for (;;) asm volatile ("hlt");
}