section .asm

global insb
global insw
global outb
global outw

insb:
    push ebp
    mov ebp, esp

    xor eax, eax ;set eax to zero
    mov edx, [ebp+8] ;move the 1 byte value into edx
    in al, dx ;move the value to al (part of the eax register)

    pop ebp
    ret


insw:
    push ebp
    mov ebp, esp

    xor eax, eax ;set eax to zero
    mov edx, [ebp+8] ;move the 1 byte value into edx
    in ax, dx ;move the value to al (part of the eax register)

    pop ebp
    ret

outb:
    push ebp
    mov ebp, esp  

    mov eax, [ebp+12] 
    mov edx, [ebp+8]
    out dx, al

    pop ebp
    ret

outw:
    push ebp
    mov ebp, esp  

    mov eax, [ebp+12] 
    mov edx, [ebp+8]
    out dx, ax

    pop ebp
    ret