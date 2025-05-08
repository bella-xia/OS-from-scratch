section .asm

extern idt_zero

global idt_load
global isr0

isr0:
    pusha
    call idt_zero
    popa
    iret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8] ;fist item in the function
    lidt [ebx]

    pop ebp
    ret