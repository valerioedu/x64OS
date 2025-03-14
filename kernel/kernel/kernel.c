#include "kernel.h"
#include "../drivers/keyboard/keyboard.h"

extern int fpu_init();

void kernel_main() {
    vga_init();
    kprint("Vga initialized. Hello, World!\n\tx64OS\n");
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
    kprint("Keyboard initialized\n");
    int a = fpu_init();
    if (a == 0) kprint("Floating Point Unit initialized\n");
    for (;;) asm volatile ("hlt");
}