#ifndef PIT_H
#define PIT_H

#include "../../../lib/definitions.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_BASE_FRQ 1193182UL

static inline void pit_set_frequency(uint32_t hz) {
    if (hz == 0) {
        hz = 100;
    }

    uint32_t divisor = PIT_BASE_FRQ / hz;

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

#endif