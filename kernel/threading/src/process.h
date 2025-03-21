#ifndef PROCESS_H
#define PROCESS_H

#include "../../../lib/definitions.h"

#define MAX_PROCESSES 256

typedef enum ProcessState {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef enum ProcessPriority {
    IDLE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
} ProcessPriority;

typedef struct __attribute__((packed)) CpuState {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} CpuState;

typedef struct FpuState {
    uint8_t buffer[512];
    uint16_t status;
    uint16_t control;
    uint16_t tag;
    uint16_t opcode;
    uint64_t ip_offset;
    uint64_t ip_selector;
    uint64_t data_offset;
    uint64_t data_selector;
    uint8_t st[8][10];
    uint8_t mm[8][8];
    uint8_t xmm[8][16];
    uint8_t padding[96];
} FpuState;

typedef struct Process {        //basic, but should do the job for now
    ProcessPriority priority;
    CpuState cpu_state;
    ProcessState state;
    uint64_t cr3;
    char* name;
    uint16_t pid;
    struct Process* next;
    struct Process* prev;
} Process;

typedef struct ProcessQueue {
    Process* head;
    Process* tail;
} ProcessQueue;

#endif