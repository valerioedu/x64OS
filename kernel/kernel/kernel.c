#include "kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../../lib/r0io.h"

extern int fpu_init();

void kernel_main() {
    vga_init();
    kprint("Vga initialized. Hello, World!\n\tx64OS\n");
    heap_init();
    kprint("Heap initialized\n");
    idt_init();
    kprint("Interrupts enabled\n");
    keyboard_init();
    kprint("Keyboard initialized\n");
    ide_init();
    int a = fpu_init();
    if (a == 0) kprint("Floating Point Unit initialized\n");
    for (;;) asm volatile ("hlt");
}