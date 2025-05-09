section .asm

extern idt_zero
extern idt_twenty
extern idt_twenty_one
extern idt_no_int

global idt_load
global isr0
global isr20
global isr21
global no_interrupt
global enable_interrupts
global disable_interrupts

isr0:
    cli
    pushad  
    call idt_zero
    popad
    sti
    iret

isr21:
    cli
    pushad
    call idt_twenty_one
    popad
    sti
    iret

isr20:
    cli
    pushad
    call idt_twenty
    popad
    sti
    iret

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8] ;fist item in the function
    lidt [ebx]

    pop ebp
    ret

no_interrupt:
    cli
    pushad
    call idt_no_int
    popad
    sti
    iret