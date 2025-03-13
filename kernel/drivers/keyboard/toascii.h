#ifndef TOASCII_H
#define TOASCII_H

#include "../../../lib/definitions.h"

char scancode_to_ascii(uint8_t scancode) {
    //static int shift_pressed = 0;

    // 0x2A = Left Shift (press), 0x36 = Right Shift (press)
    // 0xAA = Left Shift (release), 0xB6 = Right Shift (release)
    //if (scancode == 0x2A || scancode == 0x36) {
    //    shift_pressed = 1;
    //    return 0; 
    //} else if (scancode == (0x2A + 0x80) || scancode == (0x36 + 0x80)) {
    //    shift_pressed = 0;
    //    return 0;
    //}

    // Now handle "press" scancodes directly
    switch (scancode) {
        //case 0x01: return 27;        // Esc
        case 0x0E: return '\b';      // Backspace
        //case 0x0F: return '\t';      // Tab
        case 0x1C: return '\n';      // Enter
        case 0x39: return ' ';       // Space bar

        // Letters
        case 0x1E: return 'a';
        case 0x30: return 'b';
        case 0x2E: return 'c';
        case 0x20: return 'd';
        case 0x12: return 'e';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x17: return 'i';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x32: return 'm';
        case 0x31: return 'n';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x10: return 'q';
        case 0x13: return 'r';
        case 0x1F: return 's';
        case 0x14: return 't';
        case 0x16: return 'u';
        case 0x2F: return 'v';
        case 0x11: return 'w';
        case 0x2D: return 'x';
        case 0x15: return 'y';
        case 0x2C: return 'z';

        // Number row (unshifted). For shifted symbols (e.g. '!', '@'),
        // you'll need to do additional logic if shift_pressed == 1.
        //case 0x02: return '1';
        //case 0x03: return '2';
        //case 0x04: return '3';
        //case 0x05: return '4';
        //case 0x06: return '5';
        //case 0x07: return '6';
        //case 0x08: return '7';
        //case 0x09: return '8';
        //case 0x0A: return '9';
        //case 0x0B: return '0';

        // A few punctuation keys (unshifted)
        //case 0x1A: return '[';
        //case 0x1B: return ']';
        //case 0x2B: return '\\';
        //case 0x27: return ';';
        //case 0x28: return '\'';
        //case 0x33: return ',';
        //case 0x34: return '.';
        //case 0x35: return '/';
        //case 0x29: return '`';

        default:
            // Unhandled scancode
            //return 0;
    }
}

#endif