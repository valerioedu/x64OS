#ifndef DEVICES_H
#define DEVICES_H

#include "../../../lib/definitions.h"

typedef struct Device {
    char* name;
    void (*init)();
    void (*read)(uint32_t lba, uint8_t* buffer, uint32_t count);
    void (*write)(uint32_t lba, uint8_t* buffer, uint32_t count);
} Device;

#endif