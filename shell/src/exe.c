#include "../../kernel/threading/src/binary.h"
#include "commands.h"

int exec(const char* path) {
    uint64_t entry_point;
    int result = load_binary(path, &entry_point);
    
    if (result != 0) {
        kprintf("Failed to load binary: %d\n", result);
        return result;
    }
    
    typedef int (*binary_entry_t)(void);
    binary_entry_t entry = (binary_entry_t)entry_point;
    return entry();
}