#include "../lib/definitions.h"
#include "src/commands.h"
#include "../lib/r0io.h"
#include "../kernel/fs/fs.h"
#include "../kernel/drivers/vga/vga.h"
#include "../kernel/drivers/keyboard/keyboard.h"

Command commands[] = {
    {"help", help},
    {"clear", clear},
    {"mkdir", mkdir},
    {"cd", cd},
    {"ls", ls},
    {"touch", touch},
    {"rm", rm},
    {"rmdir", rmdir}
};

void shell_init() {
    kprintf("-----------------------\n");
    kprintf("ProtoBash Shell\n");
    kprintf("Type 'help' for a list of commands\n");
    kprintf("-----------------------\n\n");
}

void shell_loop() {
    vga_clear();
    vga_move_cursor(0, 0);
    shell_init();
    while (1) {
        kprintf("x64OS");
        set_color(WHITE);
        kprintf(":");
        set_color(LIGHT_CYAN);
        kprintf("%s$ ", get_current_path());
        set_color(LIGHT_GREEN);
        char command_line[512] = {0};
        kfgets(command_line, 512, 0);
        while (command_line[0] == 0 ) asm volatile ("hlt");
        if (command_line) {
            char command[64] = {0};
            char args[512] = {0};
            
            int i = 0, j = 0;
            while (command_line[i] && command_line[i] != ' ' && i < 63) {
                command[i] = command_line[i];
                i++;
            }
            command[i] = '\0';
            
            while (command_line[i] && command_line[i] == ' ') {
                i++;
            }
            
            while (command_line[i] && j < 511) {
                args[j] = command_line[i];
                i++; j++;
            }
            args[j] = '\0';

            bool found = false;
            for (size_t i = 0; i < sizeof(commands) / sizeof(Command); i++) {
                if (strcmp(command, commands[i].name) == 0) {
                    commands[i].function(args);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                // Try to execute as binary
                char full_path[512] = {0};
                if (command[0] != '/') {
                    // Relative path, add current directory
                    kprintf(full_path, "%s/%s", get_current_path(), command);
                } else {
                    strcpy(full_path, command);
                }
                
                if (exec(full_path) != 0) {
                    kprintf("Command not found: %s\n", command);
                }
            }
        }
    }
}