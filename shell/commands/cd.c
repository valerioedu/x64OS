#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "commands.h"

void cd(char* args) {
    if (args[0] == '\0') {
        kprint("Usage: cd <directory-name>\n");
    } else {
        int result = change_dir(args);
        if (result != 0) {
            kprintf("Failed to change to directory '%s'\n", args);
        }
    }
}