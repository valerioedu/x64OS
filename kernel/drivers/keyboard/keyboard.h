#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../../../lib/definitions.h"

#define KEY_NULL 0
#define KEY_ESC 27
#define KEY_BACKSPACE '\b'
#define KEY_TAB '\t'
#define KEY_ENTER '\n'
#define KEY_RETURN '\r'

#define KEY_INSERT 0x90
#define KEY_DELETE 0x91
#define KEY_HOME 0x92
#define KEY_END 0x93
#define KEY_PAGE_UP 0x94
#define KEY_PAGE_DOWN 0x95
#define KEY_LEFT 0x4B
#define KEY_UP 0x48
#define KEY_RIGHT 0x4D
#define KEY_DOWN 0x50

#define KEY_F1 0x80
#define KEY_F2 0x81
#define KEY_F3 0x82
#define KEY_F4 0x83
#define KEY_F5 0x84
#define KEY_F6 0x85
#define KEY_F7 0x86
#define KEY_F8 0x87
#define KEY_F9 0x88
#define KEY_F10 0x89
#define KEY_F11 0x8A
#define KEY_F12 0x8B

#define KEY_CTRL 0x1D
#define KEY_ALT 0x38
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_CAPS_LOCK 0x3A

#define KEYBOARD_RELEASE 0x80

extern char scancode_set1[128];

#define KEY_IS_PRESS(_scancode) (!(_scancode & KEYBOARD_RELEASE))
#define KEY_IS_RELEASE(_scancode) (_scancode & KEYBOARD_RELEASE)
#define KEY_SCANCODE(_scancode) (_scancode & 0x7F)
#define KEY_CHAR(scancode) (__extension__({ \
    uint8_t sc = KEY_SCANCODE(scancode); \
    sc < sizeof(scancode_set1) ? scancode_set1[sc] : 0; \
}))

extern volatile char g_last_char;

void keyboard_init();
void isr_keyboard_handler();
uint8_t keyboard_get_scancode();
char keyboard_get_char();
char* read();

#endif
