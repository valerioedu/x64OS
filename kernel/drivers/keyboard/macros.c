#include "macros.h"
#include "keyboard.h"

keyboard_macro_t macros[MAX_MACROS];
int macro_count = 0;

char highlight_buffer[256];
int highlight_index = 0;

int keyboard_register_macro(bool ctrl, bool alt, bool shift, char key, void (*handler)(void)) {
    if (macro_count >= MAX_MACROS) return -1;
    keyboard_macro_t macro;
    macro.ctrl = ctrl;
    macro.alt = alt;
    macro.shift = shift;
    macro.key = key;
    macro.handler = handler;
    macros[macro_count++] = macro;
    return 0;
}

int keyboard_unregister_macro(bool ctrl, bool alt, bool shift, char key) {
    for (int i = 0; i < macro_count; i++) {
        if (macros[i].ctrl == ctrl && macros[i].alt == alt && macros[i].shift == shift && macros[i].key == key) {
            for (int j = i; j < macro_count - 1; j++) {
                macros[j] = macros[j + 1];
            }
            macro_count--;
            return 0;
        }
    }
    return -1;
}

void check_for_macros() {
    for (int i = 0; i < macro_count; i++) {
        if (macros[i].ctrl == g_ctrl_pressed && macros[i].alt == g_alt_pressed &&
            macros[i].shift == g_shift_pressed && macros[i].key == g_last_char) {
            macros[i].handler();
        }
    }
}

void highlight_char(char c) {
    highlight_buffer[highlight_index++] = c;
}

void shift_left_arrow() {
    if (highlight_index > 0) {
        highlight_index--;
        for (int i = 0; i < highlight_index; i++) {
            highlight_char(highlight_buffer[i]);
        }
    }
}

void shift_right_arrow() {
    if (highlight_index < strlen(highlight_buffer)) {
        highlight_index++;
        for (int i = 0; i < highlight_index; i++) {
            highlight_char(highlight_buffer[i]);
        }
    }
}

void ctrl_c() {
    int copy_len = highlight_index;
    if (copy_len >= 256) {
        copy_len = 256 - 1;
    }

    for (int i = 0; i < copy_len; i++) {
        highlight_buffer[i] = highlight_buffer[i];
    }
    highlight_buffer[copy_len] = '\0';
}

void ctrl_v() {
    highlight_buffer[highlight_index+1] = '\0';
    kprint(highlight_buffer);
}

void init_macros() {
    keyboard_register_macro(0, 0, 1, KEY_LEFT, shift_left_arrow);
    keyboard_register_macro(0, 0, 1, KEY_RIGHT, shift_right_arrow);
    keyboard_register_macro(1, 0, 0, 46, ctrl_c);
    keyboard_register_macro(1, 0, 0, 47, ctrl_v);
}