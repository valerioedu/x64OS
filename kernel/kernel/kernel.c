#include "kernel.h"
#include "../drivers/keyboard/keyboard.h"
#include "../../lib/r0io.h"
#include "../fs/fs.h"

extern int fpu_init();

void ProtoBash() {
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
            // Parse the command and arguments
            char command[64] = {0};
            char args[512] = {0}; // Larger buffer for arguments
            
            // Simple command parsing - extract first word as command
            int i = 0, j = 0;
            while (command_line[i] && command_line[i] != ' ' && i < 63) {
                command[i] = command_line[i];
                i++;
            }
            command[i] = '\0';
            
            // Skip spaces
            while (command_line[i] && command_line[i] == ' ') {
                i++;
            }
            
            // Extract all remaining arguments
            while (command_line[i] && j < 511) {
                args[j] = command_line[i];
                i++; j++;
            }
            args[j] = '\0';
            
            if (strcmp(command, "help") == 0) {
                set_color(LIGHT_RED);
                kprint("Available commands:\n");
                set_color(LIGHT_BROWN);
                kprint("  help ");
                set_color(LIGHT_GREEN);
                kprintcolor("-", WHITE);
                kprint(" Show this help message\n");
                set_color(LIGHT_BROWN);
                kprint("  clear ");
                set_color(LIGHT_GREEN);
                kprintcolor("-", WHITE);
                kprint(" Clear the screen\n");
                set_color(LIGHT_BROWN);
                kprint("  mkdir ");
                set_color(LIGHT_MAGENTA);
                kprint("<name>");
                set_color(LIGHT_GREEN);
                kprintcolor(" -", WHITE);
                kprint(" Create a new directory\n");
                set_color(LIGHT_BROWN);
                kprint("  cd ");
                set_color(LIGHT_MAGENTA);
                kprint("<name>");
                set_color(LIGHT_GREEN);
                kprintcolor(" -", WHITE);
                kprint(" Change current directory\n");
                set_color(LIGHT_BROWN);
                kprint("  ls ");
                set_color(LIGHT_GREEN);
                kprintcolor("-", WHITE);
                kprint(" List directory contents\n");
                // Add new commands to help
                set_color(LIGHT_BROWN);
                kprint("  touch ");
                set_color(LIGHT_MAGENTA);
                kprint("<filename>");
                set_color(LIGHT_GREEN);
                kprintcolor(" -", WHITE);
                kprint(" Create an empty file\n");
                set_color(LIGHT_BROWN);
                kprint("  rm ");
                set_color(LIGHT_MAGENTA);
                kprint("<filename>");
                set_color(LIGHT_GREEN);
                kprintcolor(" -", WHITE);
                kprint(" Remove a file\n");
                set_color(LIGHT_BROWN);
                kprint("  write ");
                set_color(LIGHT_MAGENTA);
                kprint("<filename> <text>");
                set_color(LIGHT_GREEN);
                kprintcolor(" -", WHITE);
                kprint(" Write text to a file\n");
                set_color(LIGHT_BROWN);
                kprint("  exit ");
                set_color(LIGHT_GREEN);
                kprintcolor("-", WHITE);
                kprint(" Exit the shell\n");
            } else if (strcmp(command, "clear") == 0) {
                vga_clear();
                vga_move_cursor(0, 0);
            } else if (strcmp(command, "exit") == 0) {
                break;
            } else if (strcmp(command, "mkdir") == 0) {
                // Existing mkdir code
                // [Keep the existing mkdir implementation here]
                if (args[0] == '\0') {
                    kprint("Usage: mkdir <directory-name>\n");
                } else {
                    // Check for invalid characters in directory name
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
                        kprintf("Invalid directory name: '%s'\n", args);
                        kprintf("Names cannot contain: / \\ : * ? \" < > |\n");
                        continue;
                    }

                    // Check name length
                    if (strlen(args) >= DISKFS_MAX_NAME) {
                        kprintf("Directory name too long (max %d characters)\n", DISKFS_MAX_NAME - 1);
                        continue;
                    }

                    Inode* current_dir = get_current_dir();
                    if (!current_dir) {
                        kprintf("Error: Cannot access current directory\n");
                        continue;
                    }

                    if (!current_dir->ops || !current_dir->ops->mkdir) {
                        kprintf("Error: mkdir operation not available\n");
                        continue;
                    }

                    // Check if directory already exists
                    Inode* check = NULL;
                    if (current_dir->ops->lookup(current_dir, args, &check)) {
                        kprintf("Error: '%s' already exists\n", args);
                        kfree(check); // Free the inode
                        continue;
                    }

                    // Create the directory
                    int result = current_dir->ops->mkdir(current_dir, args, 0755);
                    if (result) {
                        kprintf("Directory '%s' created successfully\n", args);
                    } else {
                        kprintf("Failed to create directory '%s'\n", args);
                        // Add debugging info
                        DiskfsInfo* dfs = (DiskfsInfo*)current_dir->sb->fs_specific;
                        InodeCacheEntry* dir_ice = (InodeCacheEntry*)current_dir->fs_specific;
                        kprintf("Debug: inode_count=%d, free_inodes=%d\n",
                               dfs->super.inode_count,
                               dfs->super.total_inodes - dfs->super.inode_count);
                    }
                }
            } else if (strcmp(command, "cd") == 0) {
                // Existing cd code
                // [Keep the existing cd implementation here]
                if (args[0] == '\0') {
                    kprint("Usage: cd <directory-name>\n");
                } else {
                    int result = change_dir(args);
                    if (result != 0) {
                        kprintf("Failed to change to directory '%s'\n", args);
                    }
                }
            } else if (strcmp(command, "ls") == 0) {
                Inode* current_dir = get_current_dir();
                if (!current_dir) {
                    kprint("ls: Cannot access current directory\n");
                } else {
                    // Use the VFS API to read directory entries
                    DiskfsInfo* dfs = (DiskfsInfo*)current_dir->sb->fs_specific;
                    InodeCacheEntry* dir_ice = (InodeCacheEntry*)current_dir->fs_specific;

                    // Check if it's a directory
                    if (!(dir_ice->inode.flags & INODE_DIRECTORY)) {
                        kprint("ls: Current path is not a directory\n");
                    } else {
                        kprintf("Directory contents:\n");

                        // Process directory blocks to list entries
                        for (uint32_t block_idx = 0; block_idx < DIRECT_BLOCKS; block_idx++) {
                            uint32_t block_num = dir_ice->inode.direct[block_idx];
                            if (block_num == 0) continue;

                            BlockCacheEntry* bce = get_block(dfs, block_num);
                            if (!bce) continue;

                            // Parse directory entries
                            DiskfsDirectory* dir_header = (DiskfsDirectory*)bce->data;
                            DiskfsDirEntry* entries = (DiskfsDirEntry*)(bce->data + sizeof(DiskfsDirectory));

                            // List entries in this block
                            for (uint32_t i = 0; i < dir_header->entry_count; i++) {
                                // Determine if it's a directory
                                bool is_dir = (entries[i].type == INODE_DIRECTORY);

                                // Set color based on entry type (directory or file)
                                set_color(is_dir ? LIGHT_CYAN : LIGHT_GREEN);

                                // Print the entry name with a trailing slash for directories
                                kprintf("  %s%s\n", entries[i].name, is_dir ? "/" : "");
                            }

                            // Check for linked blocks
                            uint32_t next_block = dir_header->next_block;
                            release_block(bce);

                            // Process any linked blocks
                            while (next_block != 0) {
                                BlockCacheEntry* next_bce = get_block(dfs, next_block);
                                if (!next_bce) break;

                                DiskfsDirectory* next_dir = (DiskfsDirectory*)next_bce->data;
                                DiskfsDirEntry* next_entries = (DiskfsDirEntry*)(next_bce->data + sizeof(DiskfsDirectory));

                                for (uint32_t i = 0; i < next_dir->entry_count; i++) {
                                    bool is_dir = (next_entries[i].type == INODE_DIRECTORY);
                                    set_color(is_dir ? LIGHT_CYAN : LIGHT_GREEN);
                                    kprintf("  %s%s\n", next_entries[i].name, is_dir ? "/" : "");
                                }

                                uint32_t tmp_next = next_dir->next_block;
                                release_block(next_bce);
                                next_block = tmp_next;
                            }
                        }

                        // Reset color
                        set_color(LIGHT_GREEN);
                    }
                }
            } 
            
            // NEW COMMANDS START HERE
            else if (strcmp(command, "touch") == 0) {
                if (args[0] == '\0') {
                    kprint("Usage: touch <filename>\n");
                } else {
                    // Validate filename
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
                        continue;
                    }
                    
                    // Check name length
                    if (strlen(args) >= DISKFS_MAX_NAME) {
                        kprintf("Filename too long (max %d characters)\n", DISKFS_MAX_NAME - 1);
                        continue;
                    }
                    
                    Inode* current_dir = get_current_dir();
                    if (!current_dir) {
                        kprintf("Error: Cannot access current directory\n");
                        continue;
                    }
                    
                    // Check if file already exists
                    Inode* check = NULL;
                    if (current_dir->ops->lookup(current_dir, args, &check)) {
                        kprintf("Error: '%s' already exists\n", args);
                        kfree(check); // Free the inode
                        continue;
                    }
                    
                    // Create the file
                    int result = current_dir->ops->create(current_dir, args, 0644); // rw-r--r--
                    if (result) {
                        kprintf("File '%s' created successfully\n", args);
                    } else {
                        kprintf("Failed to create file '%s'\n", args);
                    }
                }
            } else if (strcmp(command, "rm") == 0) {
                if (args[0] == '\0') {
                    kprint("Usage: rm <filename>\n");
                } else {
                    Inode* current_dir = get_current_dir();
                    if (!current_dir) {
                        kprintf("Error: Cannot access current directory\n");
                        continue;
                    }
                    
                    // Check if file exists
                    Inode* check = NULL;
                    if (!current_dir->ops->lookup(current_dir, args, &check)) {
                        kprintf("Error: '%s' does not exist\n", args);
                        continue;
                    }
                    
                    // Free the check inode
                    kfree(check);
                    
                    // Delete the file
                    int result = current_dir->ops->delete(current_dir, args);
                    if (result) {
                        kprintf("'%s' deleted successfully\n", args);
                    } else {
                        kprintf("Failed to delete '%s'\n", args);
                    }
                }
            } else if (strcmp(command, "write") == 0) {
                // For write, we need to parse filename and content
                char filename[64] = {0};
                char content[448] = {0}; // Remaining space in args buffer
                
                // Extract filename (first word in args)
                i = 0;
                while (args[i] && args[i] != ' ' && i < 63) {
                    filename[i] = args[i];
                    i++;
                }
                filename[i] = '\0';
                
                // Skip spaces
                while (args[i] && args[i] == ' ') {
                    i++;
                }
                
                // Extract content (rest of args)
                j = 0;
                while (args[i] && j < 447) {
                    content[j] = args[i];
                    i++; j++;
                }
                content[j] = '\0';
                
                if (filename[0] == '\0') {
                    kprint("Usage: write <filename> <content>\n");
                } else if (content[0] == '\0') {
                    kprint("No content specified for write command\n");
                } else {
                    Inode* current_dir = get_current_dir();
                    if (!current_dir) {
                        kprintf("Error: Cannot access current directory\n");
                        continue;
                    }
                    
                    // Create or overwrite the file
                    Inode* file_inode = NULL;
                    bool exists = false;
                    
                    // Check if file exists
                    if (current_dir->ops->lookup(current_dir, filename, &file_inode)) {
                        exists = true;
                    } else {
                        // Create file if it doesn't exist
                        if (!current_dir->ops->create(current_dir, filename, 0644)) {
                            kprintf("Failed to create file '%s'\n", filename);
                            continue;
                        }
                        
                        // Get the new file's inode
                        if (!current_dir->ops->lookup(current_dir, filename, &file_inode)) {
                            kprintf("Error accessing created file\n");
                            continue;
                        }
                    }
                    
                    // Write content to the file
                    uint64_t bytes_written = file_inode->ops->write(file_inode, 0, content, strlen(content));
                    
                    if (bytes_written == strlen(content)) {
                        kprintf("Wrote %d bytes to '%s'\n", bytes_written, filename);
                    } else {
                        kprintf("Error: Wrote only %d of %d bytes to file\n", 
                                bytes_written, strlen(content));
                    }
                    
                    // Free the file inode
                    kfree(file_inode);
                }
            } 
            else if (strcmp(command, "read") == 0) {
                if (args[0] == '\0') {
                    kprint("Usage: read <filename>\n");
                } else {
                    Inode* current_dir = get_current_dir();
                    if (!current_dir) {
                        kprintf("Error: Cannot access current directory\n");
                        continue;
                    }
                    
                    // Check if file exists
                    Inode* file_inode = NULL;
                    if (!current_dir->ops->lookup(current_dir, args, &file_inode)) {
                        kprintf("Error: '%s' does not exist\n", args);
                        continue;
                    }
                    
                    // Get file size to allocate buffer
                    uint64_t file_size = file_inode->size;
                    
                    if (file_size == 0) {
                        kprintf("File '%s' is empty\n", args);
                        kfree(file_inode);
                        continue;
                    }
                    
                    // Cap file size to prevent excessive memory usage
                    if (file_size > 4096) {
                        kprintf("Warning: File size is %d bytes, showing first 4096 bytes\n", file_size);
                        file_size = 4096;
                    }
                    
                    // Allocate buffer for file contents
                    char* buffer = (char*)kmalloc(file_size + 1);
                    if (!buffer) {
                        kprintf("Error: Failed to allocate memory for file contents\n");
                        kfree(file_inode);
                        continue;
                    }
                    
                    // Read file contents
                    uint64_t bytes_read = file_inode->ops->read(file_inode, 0, buffer, file_size);
                    buffer[bytes_read] = '\0'; // Null terminate the buffer
                    
                    // Display file contents
                    if (bytes_read > 0) {
                        set_color(WHITE);
                        kprintf("--- Contents of %s (%d bytes) ---\n", args, bytes_read);
                        set_color(LIGHT_CYAN);
                        kprint(buffer);
                        set_color(WHITE);
                        kprintf("\n--- End of %s ---\n", args);
                        set_color(LIGHT_GREEN);
                    } else {
                        kprintf("Error: Failed to read file '%s'\n", args);
                    }
                    
                    // Free resources
                    kfree(buffer);
                    kfree(file_inode);
                }
            }
            else {
                kprint("Unknown command: ");
                kprint(command);
                kprint("\n");
            }
        }
    }
}

void kernel_main() {
    vga_init();
    kprint("Vga initialized\n");
    heap_init();
    kprint("Heap initialized\n");
    idt_init();
    kprint("Interrupts enabled\n");
    keyboard_init();
    kprint("Keyboard initialized\n");
    ide_init();
    int a = fpu_init();
    if (a == 0) kprint("Floating Point Unit initialized\n");
    fs_init();
    Inode* root = get_root();    
    ProtoBash();
    for (;;) asm volatile ("hlt");
}