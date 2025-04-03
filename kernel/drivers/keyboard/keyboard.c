#include "keyboard.h"
#include "../../cpu/src/pic.h"
#include "../../../lib/definitions.h"
#include "../vga/vga.h"
//#include "macros.h"

#define BUFFER_SIZE 2048

volatile uint8_t g_last_scancode = 0;
volatile char    g_last_char     = 0;
volatile uint8_t g_shift_pressed = 0;
volatile uint8_t g_ctrl_pressed = 0;
volatile uint8_t g_alt_pressed = 0;
volatile uint8_t g_caps_lock_on = 0;
volatile uint8_t g_arrow_key_pressed = 0;
uint8_t g_macro_count = 0;

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
    //init_macros();        I will insert it in the bash
    g_shift_pressed = 0;
    g_ctrl_pressed = 0;
    g_alt_pressed = 0;
    g_caps_lock_on = 0;
    g_last_char = 0;
    g_macro_count = 0;
    g_arrow_key_pressed = 0;
}

void isr_keyboard_handler() {
    uint8_t scancode = inb(0x60);
    g_last_scancode = scancode;

    if (scancode == (KEY_LSHIFT | KEYBOARD_RELEASE) ||
        scancode == (KEY_RSHIFT | KEYBOARD_RELEASE)) {
        g_shift_pressed = 0;
    } else if (scancode == (KEY_CTRL | KEYBOARD_RELEASE)) {
        g_ctrl_pressed = 0;
    } else if (scancode == (KEY_ALT | KEYBOARD_RELEASE)) {
        g_alt_pressed = 0;
    } else if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        g_shift_pressed = 1;
    } else if (scancode == KEY_CTRL) {
        g_ctrl_pressed = 1;
    } else if (scancode == KEY_ALT) {
        g_alt_pressed = 1;
    } else if (scancode == KEY_CAPS_LOCK) {
        g_caps_lock_on = !g_caps_lock_on;
    } else if (KEY_IS_PRESS(scancode)) {
        uint8_t sc = KEY_SCANCODE(scancode);

        if (sc == KEY_LEFT) {
            g_last_char = KEY_LEFT;
        } else if (sc == KEY_RIGHT) {
            g_last_char = KEY_RIGHT;
        } else if (sc == KEY_UP) {
            g_last_char = KEY_UP;
        } else if (sc == KEY_DOWN) {
            g_last_char = KEY_DOWN;
        } else if (g_shift_pressed) {
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

        if (g_ctrl_pressed &&
            g_last_char >= 'a' && g_last_char <= 'z') {
            g_last_char = g_last_char - 'a' + 1;
        }

        //check_for_macros();
    }

    pic_send_eoi(1);
}


uint8_t keyboard_get_scancode() {
    return g_last_scancode;
}

char keyboard_get_char() {
    return g_last_char;
}

char* keyboard_read_line() {
    static char buffer[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = '\0';
    }

    int length = 0;
    int index  = 0;
    
    int xCursor, yCursor;
    vga_get_cursor(&xCursor, &yCursor);

    while (1) {
        char c;
        while ((c = keyboard_get_char()) == 0) asm volatile ("hlt");

        g_last_char = 0;

        if (c == KEY_ENTER || c == KEY_RETURN) {
            buffer[length] = '\0';
            vga_putc('\n');
            break;
        } else if (c == KEY_BACKSPACE) {
            if (index > 0) {
                index--;

                for (int i = index; i < length - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                length--;
                buffer[length] = '\0';

                if (xCursor > 0) {
                    xCursor--;
                } 

                vga_move_cursor(xCursor, yCursor);

                for (int i = index; i < length; i++) {
                    vga_putc(buffer[i]);
                }
                vga_putc(' ');
                vga_move_cursor(xCursor, yCursor);
            }
        }
        else if (c == KEY_LEFT) {
            if (index > 0) {
                index--;
                if (xCursor > 0) {
                    xCursor--;
                }
                vga_move_cursor(xCursor, yCursor);
            }
        }
        else if (c == KEY_RIGHT) {
            if (index < length) {
                index++;
                xCursor++;
                if (xCursor >= VGA_WIDTH) {
                    xCursor = VGA_WIDTH - 1;
                }
                vga_move_cursor(xCursor, yCursor);
            }
        }
        else if (c >= ' ' && c <= '~') {
            if (length < (BUFFER_SIZE - 1)) {
                for (int i = length; i >= index; i--) {
                    buffer[i + 1] = buffer[i];
                }
                buffer[index] = c;
                length++;
                index++;

                vga_putc(c);

                xCursor++;
                if (xCursor >= VGA_WIDTH) {
                    xCursor = VGA_WIDTH - 1;
                }

                for (int i = index; i < length; i++) {
                    vga_putc(buffer[i]);
                }

                int currentX, currentY;
                vga_get_cursor(&currentX, &currentY);
                vga_move_cursor(xCursor, yCursor);
            }
        }
    }

    return buffer;
}
