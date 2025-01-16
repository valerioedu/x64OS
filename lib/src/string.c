#include "../definitions.h"

int strlen(const char *str) {
    int len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}