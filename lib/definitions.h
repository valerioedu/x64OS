#ifndef DEFINITIONS_H
#define DEFINITIONS_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;
typedef long long ssize_t;
typedef uint64_t uintptr_t;
typedef int mode_t;

#define NULL ((void*)0)

#define true 1
#define false 0
#define bool _Bool

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a" (val), "Nd" (port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

int strlen(const char *str);
void kprint(const char* str);
void kprintf(const char* format, ...);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* dest, int c, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strchr(const char* str, int c);
char *strtok(char* str, const char* delim);
char *strcat(char *dest, const char *src);
char* strrchr(const char* str, int c);

#endif