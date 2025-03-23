#ifndef VGA_H
#define VGA_H

#include "../../../lib/definitions.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

#define VGA_ADDRESS 0xB8000

void vga_get_cursor(int *x, int *y);
void vga_move_cursor(int x, int y);
void vga_clear();
void vga_init();
void vga_putc(char c);

#endif