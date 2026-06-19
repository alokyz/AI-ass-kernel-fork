bits 32

extern _bss_start
extern _bss_end
extern kernel_main

section .multiboot
    align 4
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)
    dd 0
    dd 1024
    dd 768
    dd 32

section .bss
    align 16
    stack_bottom:
        resb 16384
    stack_top:

section .text
    global _start

_start:
    cli
    mov esp, stack_top

    push dword 0
    push dword 0
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
