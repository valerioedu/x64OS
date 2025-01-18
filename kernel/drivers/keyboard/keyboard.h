#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../../../lib/definitions.h"
#include "../../cpu/interrupts.h"
#include "../../cpu/src/pic.h"
#include "../vga/vga.h"

void isr_keyboard_handler();

#endif