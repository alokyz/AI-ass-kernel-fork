bits 16

org 0x7C00
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Load kernel FIRST (INT 13h may corrupt low memory)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 2
    mov al, 128
    xor ch, ch
    mov cl, 2
    xor dh, dh
    mov dl, 0
    int 0x13

    ; Set VESA mode 640x480x32 (simpler, more reliable)
    mov ax, 0x4F02
    mov bx, 0x0112
    int 0x10

    ; Get VBE mode info -> 0x8000
    xor ax, ax
    mov es, ax
    mov di, 0x8000
    mov ax, 0x4F01
    mov cx, 0x0112
    int 0x10

    ; Store at 0x7E00
    mov eax, [0x8028]
    mov [0x7E00], eax
    movzx eax, word [0x8012]
    mov [0x7E04], eax
    movzx eax, word [0x8014]
    mov [0x7E08], eax
    movzx eax, word [0x8010]
    mov [0x7E0C], eax
    mov dword [0x7E10], 32
    mov dword [0x7E14], 0x4F5258

    ; A20
    in al, 0x92
    or al, 2
    out 0x92, al

    cli
    lgdt [gdtr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

bits 32
pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    mov eax, 0x4F52584E
    mov ebx, 0x7E00
    jmp 0x10000

gdtr:
    dw gdt_end - gdt - 1
    dd gdt

gdt:
    dq 0
    db 0xFF,0xFF,0,0,0,0x9A,0xCF,0
    db 0xFF,0xFF,0,0,0,0x92,0xCF,0
gdt_end:

times 510-($-$$) db 0
dw 0xAA55
