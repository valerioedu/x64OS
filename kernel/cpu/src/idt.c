#include "../interrupts.h"
#include "pit.h"
#include "pic.h"

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) interrupt_frame_t;

extern void idt_load(uint64_t idt_ptr);
extern void default_isr_stub();
extern void timer_stub();
extern void keyboard_stub();

extern void exception_div_stub();
extern void exception_debug_stub();
extern void exception_nmi_stub();
extern void exception_breakpoint_stub();
extern void exception_overflow_stub();
extern void exception_bound_stub();
extern void exception_invalid_op_stub();
extern void exception_device_na_stub();
extern void exception_double_fault_stub();
extern void exception_tss_stub();
extern void exception_segment_stub();
extern void exception_stack_stub();
extern void exception_gpf_stub();
extern void exception_page_fault_stub();

idt_entry idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

void idt_set_entry(uint8_t index, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[index].offset_low  = (uint16_t)(base & 0xFFFF);
    idt[index].offset_mid  = (uint16_t)((base >> 16) & 0xFFFF);
    idt[index].offset_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt[index].selector    = sel;
    idt[index].ist         = 0;
    idt[index].attributes  = flags;
    idt[index].reserved    = 0;
}

// Generic exception handlers
void handle_exception_divide_by_zero(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Divide by zero\n");
    kprintf("RIP: 0x%x\n", frame->rip);
    
    frame->rip += 2;
    kprintf("Skipping to: 0x%x\n", frame->rip);
}

void handle_exception_debug(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Debug\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_nmi(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Non-maskable Interrupt\n");
}

void handle_exception_breakpoint(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Breakpoint\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_overflow(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Overflow\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_bound(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Bound Range Exceeded\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_invalid_op(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Invalid Opcode\n");
    kprintf("RIP: 0x%x\n", frame->rip);
    frame->rip += 1;
}

void handle_exception_device_na(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Device Not Available\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_double_fault(interrupt_frame_t* frame) {
    kprintf("CRITICAL: Double Fault\n");
    kprintf("RIP: 0x%x\n", frame->rip);
    kprintf("System halted\n");
    asm volatile("hlt");
}

void handle_exception_tss(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Invalid TSS\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_segment(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Segment Not Present\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_stack(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: Stack-Segment Fault\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_gpf(interrupt_frame_t* frame) {
    kprintf("EXCEPTION: General Protection Fault\n");
    kprintf("RIP: 0x%x\n", frame->rip);
}

void handle_exception_page_fault(interrupt_frame_t* frame) {
    uint64_t fault_address;
    asm volatile("mov %%cr2, %0" : "=r"(fault_address));
    
    kprintf("EXCEPTION: Page Fault\n");
    kprintf("Attempted to access: 0x%llx\n", fault_address);
    kprintf("RIP: 0x%x\n", frame->rip);
}

void double_fault_handler() {
    kprintf("CRITICAL: Double fault\nHalting System");
    asm volatile("hlt");
}

void divide_by_zero_handler() {
    kprintf("Divide by zero\n");
    uint64_t rip;
    asm volatile("movq 8(%%rbp), %0" : "=r"(rip));
    kprintf("%x\n", rip);
    asm volatile("hlt");
}

void idt_init() {
    asm volatile("cli");

    pic_remap();
    outb(0x64, 0xAE);

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_entry(i, (uint64_t)default_isr_stub, 0x08, 0x8E);
    }

    idt_set_entry(0, (uint64_t)exception_div_stub, 0x08, 0x8E);
    idt_set_entry(1, (uint64_t)exception_debug_stub, 0x08, 0x8E);
    idt_set_entry(2, (uint64_t)exception_nmi_stub, 0x08, 0x8E);
    idt_set_entry(3, (uint64_t)exception_breakpoint_stub, 0x08, 0x8E);
    idt_set_entry(4, (uint64_t)exception_overflow_stub, 0x08, 0x8E);
    idt_set_entry(5, (uint64_t)exception_bound_stub, 0x08, 0x8E);
    idt_set_entry(6, (uint64_t)exception_invalid_op_stub, 0x08, 0x8E);
    idt_set_entry(7, (uint64_t)exception_device_na_stub, 0x08, 0x8E);
    idt_set_entry(8, (uint64_t)exception_double_fault_stub, 0x08, 0x8E);
    idt_set_entry(10, (uint64_t)exception_tss_stub, 0x08, 0x8E);
    idt_set_entry(11, (uint64_t)exception_segment_stub, 0x08, 0x8E);
    idt_set_entry(12, (uint64_t)exception_stack_stub, 0x08, 0x8E);
    idt_set_entry(13, (uint64_t)exception_gpf_stub, 0x08, 0x8E);
    idt_set_entry(14, (uint64_t)exception_page_fault_stub, 0x08, 0x8E);

    idt_set_entry(0x20, (uint64_t)timer_stub, 0x08, 0x8E);
    idt_set_entry(0x21, (uint64_t)keyboard_stub, 0x08, 0x8E);
    
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt[0];

    pit_set_frequency(100);

    idt_load((uint64_t)&idt_ptr);

    asm volatile("sti");
}