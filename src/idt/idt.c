#include "config.h"
#include "kernel.h"
#include "idt/idt.h"
#include "memory/memory.h"


struct idt_desc idt_descriptors[PEACHOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void idt_load(struct idtr_desc *ptr);
extern void isr0(void);

void idt_zero() {
    print("divided by zero error\n");
}

void idt_set(int int_no, void *addr) {
    struct idt_desc *desc = &idt_descriptors[int_no];
    desc->offset_1 = (uint32_t) addr & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t) addr >> 16;
}

void idt_init() {
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t) idt_descriptors;

    idt_set(0, isr0);

    // load the interrupt descriptor table
    idt_load(&idtr_descriptor);

}