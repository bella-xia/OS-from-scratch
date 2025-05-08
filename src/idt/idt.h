#ifndef IDT_H
#define IDT_H

#include <stdint.h>


struct idt_desc {
    uint16_t offset_1; //offset bit 0-15
    uint16_t selector; // selector that is in our GDT
    uint8_t zero; // does nothing, unused set to zero
    uint8_t type_attr; // descriptor type and attributes
    uint16_t offset_2; // offset bits 16-31
    
} __attribute((packed));


struct idtr_desc {
    uint16_t limit; // size of the descriptor table - 1
    uint32_t base; // base address of the start of the interrrupt descriptor table

} __attribute__((packed));


void idt_init();


#endif // IDT_H