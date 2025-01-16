#include "../kernel/kernel/kernel.h"
#include "../lib/definitions.h"

void main() {
    vga_clear();
    kernel_main();
}