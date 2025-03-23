#ifndef MACROS_H
#define MACROS_H

#include "../../../lib/definitions.h"

#define MAX_MACROS 32

typedef struct keyboard_macro_t {
    bool ctrl;
    bool alt;
    bool shift;
    char key;
    void (*handler)(void);
} keyboard_macro_t;

extern char highlight_buffer[256];
extern int highlight_index;

int keyboard_register_macro(bool ctrl, bool alt, bool shift, char key, void (*handler)(void));
int keyboard_unregister_macro(bool ctrl, bool alt, bool shift, char key);
void check_for_macros();
void init_macros();

#endif