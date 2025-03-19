#include "keyboard.h"
#include "../../cpu/src/pic.h"
#include "../../../lib/definitions.h"
#include "../vga/vga.h"

#define BUFFER_SIZE 256

volatile uint8_t g_last_scancode = 0;
volatile char    g_last_char     = 0;
static volatile uint8_t g_shift_pressed = 0;
static volatile uint8_t g_ctrl_pressed = 0;
static volatile uint8_t g_alt_pressed = 0;
static volatile uint8_t g_caps_lock_on = 0;

const char scancode_set1[128] = {
    0,      KEY_ESC, '1',    '2',    '3',    '4',    '5',    '6',
    '7',    '8',    '9',    '0',    '-',    '=',    KEY_BACKSPACE, KEY_TAB,
    'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
    'o',    'p',    '[',    ']',    KEY_ENTER, 0,     'a',    's',
    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
    '\'',   '`',    0,      '\\',   'z',    'x',    'c',    'v',
    'b',    'n',    'm',    ',',    '.',    '/',    0,      '*',
    0,      ' ',    0,      KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0,     0,      KEY_HOME,
    KEY_UP, KEY_PAGE_UP, '-', KEY_LEFT, '5', KEY_RIGHT, '+', KEY_END,
    KEY_DOWN, KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE, 0, 0,   0,     KEY_F11,
    KEY_F12, 0,     0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0
};

const char scancode_shifted[128] = {
    0,      KEY_ESC, '!',    '@',    '#',    '$',    '%',    '^',
    '&',    '*',    '(',    ')',    '_',    '+',    KEY_BACKSPACE, KEY_TAB,
    'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I',
    'O',    'P',    '{',    '}',    KEY_ENTER, 0,     'A',    'S',
    'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':',
    '"',    '~',    0,      '|',    'Z',    'X',    'C',    'V',
    'B',    'N',    'M',    '<',    '>',    '?',    0,      '*',
    0,      ' ',    0,      KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0,     0,      KEY_HOME,
    KEY_UP, KEY_PAGE_UP, '-', KEY_LEFT, '5', KEY_RIGHT, '+', KEY_END,
    KEY_DOWN, KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE, 0, 0,   0,     KEY_F11,
    KEY_F12, 0,     0,      0,      0,      0,      0,      0,
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
    uint8_t scancode = inb(0x60);
    g_last_scancode = scancode;
    
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        g_shift_pressed = 1;
    } else if (scancode == (KEY_LSHIFT | KEYBOARD_RELEASE) || 
               scancode == (KEY_RSHIFT | KEYBOARD_RELEASE)) {
        g_shift_pressed = 0;
    } else if (scancode == KEY_CTRL) {
        g_ctrl_pressed = 1;
    } else if (scancode == (KEY_CTRL | KEYBOARD_RELEASE)) {
        g_ctrl_pressed = 0;
    } else if (scancode == KEY_ALT) {
        g_alt_pressed = 1;
    } else if (scancode == (KEY_ALT | KEYBOARD_RELEASE)) {
        g_alt_pressed = 0;
    } else if (scancode == KEY_CAPS_LOCK) {
        g_caps_lock_on = !g_caps_lock_on;
    } else if (KEY_IS_PRESS(scancode)) {
        uint8_t sc = KEY_SCANCODE(scancode);
        
        if (g_shift_pressed) {
            g_last_char = scancode_shifted[sc];
        } else {
            g_last_char = scancode_set1[sc];
        }
        
        if (g_caps_lock_on) {
            if ((g_last_char >= 'a' && g_last_char <= 'z') || 
                (g_last_char >= 'A' && g_last_char <= 'Z')) {
                g_last_char ^= 0x20;
            }
        }
        
        if (g_ctrl_pressed && g_last_char >= 'a' && g_last_char <= 'z') {
            g_last_char = g_last_char - 'a' + 1;
        }
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