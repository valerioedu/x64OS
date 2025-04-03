#ifndef BINARY_H
#define BINARY_H

#include "../../../lib/definitions.h"

typedef struct {
    uint32_t magic;          // Magic number to identify valid executables (e.g., 0x4F53783E "OSx>")
    uint32_t version;
    uint64_t entry_point;
    uint64_t load_address;
    uint64_t code_size;
    uint64_t data_size;
    uint64_t bss_size;
    uint32_t flags;
} binary_header_t;

#define BINARY_MAGIC 0x4F53783E

int load_binary(const char* path, uint64_t* entry_point);

#endif