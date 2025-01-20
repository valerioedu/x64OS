#include "process.h"

Process process_list[MAX_PROCESSES];
int process_count = 0;

ProcessQueue priority_queues[5];

Process* current_process = NULL;
int next_pid = 1;

void enqueue_process(Process* process) {
    int priority = process->priority;
    process->next = NULL;
    process->prev = NULL;

    if (priority_queues[priority].head == NULL) {
        priority_queues[priority].head = process;
        priority_queues[priority].tail = process;
    } else {
        process->prev = priority_queues[priority].tail;
        priority_queues[priority].tail->next = process;
        priority_queues[priority].tail = process;
    }
}

Process* dequeue_process(ProcessPriority priority) {
    Process* process = priority_queues[priority].head;
    if (!process) return NULL;
    Process* next = process->next;
    if (next) next->prev = NULL;
    else priority_queues[priority].tail = NULL;
    priority_queues[priority].head = next;
    process->prev = process->next = NULL;
    return process;
}

int highest_priority_nonempty() {
    for (int i = 4; i >= 0; i--) {
        if (priority_queues[i].head != NULL) return i;
        else return -1;
    }
}