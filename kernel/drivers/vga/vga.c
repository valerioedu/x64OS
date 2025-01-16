#include "vga.h"
#include "../../../lib/definitions.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

void vga_clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x += 2) {
            VGA_MEMORY[y * VGA_WIDTH + x] = ' ';
            VGA_MEMORY[y * VGA_WIDTH + x + 1] = (0x0F << 8);
        }
    }
}

uint16_t get_cursor_position() {
    uint16_t position = 0;
    outb(0x3D4, 0x0F);
    position |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    position |= ((uint16_t)inb(0x3D5)) << 8;
    return position;
}

void vga_move_cursor(int x, int y) {
    uint16_t position = (y * VGA_WIDTH) + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

void vga_putc(char c) {
    if (c == '\n') {
        uint16_t position = get_cursor_position();
        vga_move_cursor(0, (position / VGA_WIDTH) + 1);
        return;
    }
    if (c == '\t') {
        uint16_t position = get_cursor_position();
        for (int i = 0; i <= 4; i++) {
            vga_putc(' ');
        }
        return;
    }
    uint16_t position = get_cursor_position();
    VGA_MEMORY[position] = (uint16_t)c | (0x0A << 8);
    vga_move_cursor((position + 1) % VGA_WIDTH, (position + 1) / VGA_WIDTH);
}

void kprint(const char* str) {
    for (int i = 0; i < strlen(str); i++) {
        vga_putc(str[i]);
    }
}

void vga_init() {
    vga_clear();
    vga_move_cursor(0, 0);
}