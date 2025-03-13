#include "../interrupts.h"
#include "pit.h"
#include "pic.h"

extern void idt_load(uint64_t idt_ptr);
extern void default_isr_stub();
extern void timer_stub();
extern void keyboard_stub();

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

void idt_init() {
    asm volatile("cli");

    pic_remap();
    outb(0x64, 0xAE);


    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (i == 0x20) {
            idt_set_entry(i, (uint64_t)timer_stub, 0x08, 0x8E);
        } else if (i == 0x21) {
            idt_set_entry(i, (uint64_t)keyboard_stub, 0x08, 0x8E);
        } else {
            idt_set_entry(i, (uint64_t)default_isr_stub, 0x08, 0x8E);
        }
    }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt[0];

    pit_set_frequency(100);

    idt_load((uint64_t)&idt_ptr);

    asm volatile("sti");
}