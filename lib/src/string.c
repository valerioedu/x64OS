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

void* memset(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    while (n--) {
        *d++ = (uint8_t)c;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) {
        n--;
    }
    if (n) {
        while (--n) {
            *d++ = '\0';
        }
    }
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* d = dest + strlen(dest);
    while (n && (*d++ = *src++)) {
        n--;
    }
    if (n) {
        *d = '\0';
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

char* strtok(char *str, const char *delim) {
    static char *last = NULL;
    if (str == NULL) str = last;
    while (*str && strchr(delim, *str)) str++;
    if (*str == '\0') return NULL;
    char *start = str;
    while (*str && !strchr(delim, *str)) str++;
    if (*str) {
        *str++ = '\0';
    }
    last = str;
    return start;
}