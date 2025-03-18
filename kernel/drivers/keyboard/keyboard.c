#include "keyboard.h"
#include "../../cpu/src/pic.h"
#include "../../../lib/definitions.h"
#include "../vga/vga.h"

#define BUFFER_SIZE 256

volatile uint8_t g_last_scancode = 0;
volatile char    g_last_char     = 0;

static const char scancode_to_ascii_table[128] = {
    [0x01] = KEY_ESC,
    [0x0E] = '\b',
    [0x0F] = '\t',
    [0x1C] = '\n',
    [0x39] = ' ',

    [0x1E] = 'a',
    [0x30] = 'b',
    [0x2E] = 'c',
    [0x20] = 'd',
    [0x12] = 'e',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x17] = 'i',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x32] = 'm',
    [0x31] = 'n',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x10] = 'q',
    [0x13] = 'r',
    [0x1F] = 's',
    [0x14] = 't',
    [0x16] = 'u',
    [0x2F] = 'v',
    [0x11] = 'w',
    [0x2D] = 'x',
    [0x15] = 'y',
    [0x2C] = 'z',

    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',

    [0x1A] = '[',
    [0x1B] = ']',
    [0x2B] = '\\',
    [0x27] = ';',
    [0x28] = '\'',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x29] = '`'
};

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
        g_last_char = scancode_to_ascii_table[scancode];
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
        
        char c = keyboard_get_char();
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