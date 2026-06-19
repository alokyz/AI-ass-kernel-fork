bits 32

extern _bss_start
extern _bss_end
extern kernel_main

section .multiboot
    align 4
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)

section .text
    global _start

_start:
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    cld
    rep stosd

    mov esp, 0x90000
    push ebx
    push eax
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
