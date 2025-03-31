#include "../../lib/definitions.h"
#include "../../kernel/fs/fs.h"
#include "commands.h"

void touch(char* args) {
    if (args[0] == '\0') {
        kprint("Usage: touch <filename>\n");
    } else {
        bool valid_name = true;
        for (int i = 0; args[i]; i++) {
            if (args[i] == '/' || args[i] == '\\' || args[i] == ':' || 
                args[i] == '*' || args[i] == '?' || args[i] == '"' || 
                args[i] == '<' || args[i] == '>' || args[i] == '|') {
                valid_name = false;
                break;
            }
        }
        
        if (!valid_name) {
            kprintf("Invalid filename: '%s'\n", args);
            kprintf("Names cannot contain: / \\ : * ? \" < > |\n");
            return;
        }
        
        if (strlen(args) >= DISKFS_MAX_NAME) {
            kprintf("Filename too long (max %d characters)\n", DISKFS_MAX_NAME - 1);
            return;
        }
        
        Inode* current_dir = get_current_dir();
        if (!current_dir) {
            kprintf("Error: Cannot access current directory\n");
            return;
        }
        
        Inode* check = NULL;
        if (current_dir->ops->lookup(current_dir, args, &check)) {
            kprintf("Error: '%s' already exists\n", args);
            kfree(check);
            return;
        }
        
        int result = current_dir->ops->create(current_dir, args, 0644);
        if (result) {
            //kprintf("File '%s' created successfully\n", args);
        } else {
            kprintf("Failed to create file '%s'\n", args);
        }
    }
}