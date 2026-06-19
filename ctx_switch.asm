bits 32

section .text

; void switch_context(uint32_t* old_esp, uint32_t new_esp);
; Saves current ESP to *old_esp, loads new_esp into ESP, returns on new stack.
global switch_context
switch_context:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    pushf
    pusha
    mov [eax], esp
    mov esp, ebx
    popa
    popf
    ret

; void switch_to_user_mode(uint32_t eip, uint32_t esp);
; Sets up IRET frame and jumps to ring 3.
global switch_to_user_mode
switch_to_user_mode:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    push dword 0x23
    push ebx
    pushfd
    pop ecx
    or ecx, 0x200
    push ecx
    push dword 0x1B
    push eax
    iret
