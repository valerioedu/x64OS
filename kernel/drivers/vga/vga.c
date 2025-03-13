#include <stdarg.h>
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
    if (c == '\b') {
        uint16_t position = get_cursor_position();
        vga_move_cursor((position - 1) % VGA_WIDTH, (position - 1) / VGA_WIDTH);
        VGA_MEMORY[position] = (uint16_t)' ' | (0x0A << 8);
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

void kprint_num(int num) {
    char buf[16];
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    do {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    } while (num > 0 && i < 15);
    
    if (is_negative) {
        buf[i++] = '-';
    }
    
    buf[i] = '\0';
    
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = buf[j];
        buf[j] = buf[k];
        buf[k] = temp;
    }
    
    kprint(buf);
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    for (const char* p = format; *p != '\0'; p++) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd': {
                    int num = va_arg(args, int);
                    kprint_num(num);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    kprint(str);
                    break;
                }
                default:
                    vga_putc('%');
                    vga_putc(*p);
                    break;
            }
        } else {
            vga_putc(*p);
        }
    }
    
    va_end(args);
}

void vga_init() {
    vga_clear();
    vga_move_cursor(0, 0);
}