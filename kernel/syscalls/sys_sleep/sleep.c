#include "sleep.h"

inline void sleep(uint32_t ms) {
    uint64_t start = timer_get_ticks();
    uint64_t end = start + ms;
    
    while (timer_get_ticks() < end) {
        asm volatile ("nop");
    }
}