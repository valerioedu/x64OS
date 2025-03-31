#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "../../kernel/drivers/vga/vga.h"
#include "commands.h"

void help(char* args) {
    kprintcolor("Available commands:\n", LIGHT_RED);
    kprintcolor("  help ", LIGHT_BROWN);
    kprintcolor("-", WHITE);
    kprint(" Show this help message\n");
    kprintcolor("  clear ", LIGHT_BROWN);
    kprintcolor("-", WHITE);
    kprint(" Clear the screen\n");
    kprintcolor("  mkdir ", LIGHT_BROWN);
    kprintcolor("<name>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Create a new directory\n");
    kprintcolor("  cd ", LIGHT_BROWN);
    kprintcolor("<name>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Change current directory\n");
    kprintcolor("  ls ", LIGHT_BROWN);
    kprintcolor("-", WHITE);
    kprint(" List directory contents\n");
    kprintcolor("  touch ", LIGHT_BROWN);
    kprintcolor("<filename>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Create an empty file\n");
    kprintcolor("  rm ", LIGHT_BROWN);
    kprintcolor("<filename>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Remove a file\n");
    kprintcolor("  rmdir ", LIGHT_BROWN);
    kprintcolor("<directory>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Remove a directory\n");
    kprintcolor("  write ", LIGHT_BROWN);
    kprintcolor("<filename> <text>", LIGHT_MAGENTA);
    kprintcolor(" -", WHITE);
    kprint(" Write text to a file\n");
    kprintcolor("  exit ", LIGHT_BROWN);
    kprintcolor("-", WHITE);
    kprint(" Exit the shell\n");
}