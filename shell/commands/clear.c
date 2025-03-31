#include "../../lib/definitions.h"
#include "../../kernel/drivers/vga/vga.h"
#include "commands.h"

void clear(char* args) {
    vga_clear();
    vga_move_cursor(0, 0);
}