#ifndef KERNEL_H
#define KERNEL_H

#include "../../lib/definitions.h"
#include "../drivers/vga/vga.h"
#include "../mm/heap.h"
#include "../cpu/interrupts.h"
#include "../drivers/IDE/ide.h"

void kernel_main();

#endif