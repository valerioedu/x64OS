#include "process.h"
#include "../../mm/stack.h"

Process* process_list = NULL;
Process* current_process = NULL;

void idle_process() {
    while (1) asm volatile ("hlt");
}

Process* create_process(void(*entry_point)()) {
    Process* new_process = (Process*)kmalloc(sizeof(Process));
}