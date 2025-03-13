#include "keyboard.h"
#include "toascii.h"
#include "../../cpu/src/pic.h"
#include "../../../lib/definitions.h"
#include "../vga/vga.h"

#define BUFFER_SIZE 256

volatile uint8_t g_last_scancode = 0;
volatile char    g_last_char     = 0;

void keyboard_init() {
    outb(0x60, 0xF4);
    int timeout = 100000;
    while(timeout--) {
        if(inb(0x60) == 0xFA)
            break;
    }
}

void isr_keyboard_handler() {
    uint16_t scancode = (uint16_t)inb(0x60);
    g_last_scancode = scancode;
    if (KEY_IS_PRESS(scancode)) {
        g_last_char = KEY_SCANCODE(scancode);
    }
    
    pic_send_eoi(1);
}

uint8_t keyboard_get_scancode() {
    return g_last_scancode;
}

char keyboard_get_char() {
    return g_last_char;
}

char* read() {
    static char buffer[BUFFER_SIZE];
    int index = 0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = '\0';
    }

    while (1) {
        while (keyboard_get_char() == 0) {
        }
        
        int i = keyboard_get_scancode();
        char c = scancode_to_ascii(i);
        g_last_char = 0;

        if (c == KEY_ENTER || c == KEY_RETURN) {
            vga_putc('\n');
            buffer[index] = '\0';
            break;
        }
        else if (c == KEY_BACKSPACE) {
            if (index > 0) {
                index--;
                buffer[index] = '\0';
                vga_putc(KEY_BACKSPACE);
                vga_putc(' ');
                vga_putc(KEY_BACKSPACE);
            }
        }
        else {
            if (index < BUFFER_SIZE - 1) {
                buffer[index++] = c;
                buffer[index] = '\0';
                vga_putc(c);
            }
        }
    }

    return buffer;
}