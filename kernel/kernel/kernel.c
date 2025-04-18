#include "kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../../lib/r0io.h"
#include "../fs/fs.h"
#include "../../shell/shell.h"
#include "../drivers/PCI/pci.h"

extern int fpu_init();

void kernel_main() {
    vga_init();
    kprint("Vga initialized\n");
    heap_init();
    kprint("Heap initialized\n");
    idt_init();
    kprint("Interrupts enabled\n");
    keyboard_init();
    kprint("Keyboard initialized\n");
    ide_init();
    int a = fpu_init();
    if (a == 0) kprint("Floating Point Unit initialized\n");
    pci_init();
    fs_init();
    Inode* root = get_root();
    shell_loop();
    for (;;) asm volatile ("hlt");
}