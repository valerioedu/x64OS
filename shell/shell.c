#include "../lib/definitions.h"
#include "../kernel/kernel/kernel.h"
#include "commands/commands.h"

void shell_loop() {
    kprint("ProtoBash Shell\n");
    kprint("Type 'help' for a list of commands\n");
    while (1) {
        kprintf("x64OS");
        set_color(WHITE);
        kprintf(":");
        set_color(LIGHT_CYAN);
        kprintf("%s$ ", get_current_path());
        set_color(LIGHT_GREEN);
        char* command_line = keyboard_read_line();
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

            for (int k = 0; k < sizeof(commands) / sizeof(Command); k++) {
                if (strcmp(command, commands[k].name) == 0) {
                    commands[k].function(args);
                    break;
                }
            }
        }
    }
}