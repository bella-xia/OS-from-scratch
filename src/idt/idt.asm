section .asm

global idt_load

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8] ;fist item in the function
    lidt [ebx]

    pop ebp
    ret