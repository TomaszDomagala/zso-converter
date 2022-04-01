global call_64_from_64
global call_32_from_64
global call_32_from_32

section .text

call_64_from_64:
    mov rsi, rdi
    mov rax, 1
    mov rdi, 1
    mov rdx, 44
    syscall
    ret

call_32_from_64:
    push rbx
    mov rax, 4
    mov rbx, 1
    mov rcx, rdi
    mov rdx, 44
    int 0x80
    pop rbx
    ret

call_32_from_32:
    push rbx
    ; Stash stack
    mov [_stack_stash], rsp
    lea rsp, [rdi + 0x80]
    ; Stash "how to get back"
    mov r9, 0x3300000000
    or r9, _call32_return_64
    push r9
    push _call32_return_32
    ; Set up far return/jump
    push 0x23
    push _call_32_from_32
    db 0x48 ; REX
    retf

_call32_return_64:
    mov rsp, [_stack_stash]
    pop rbx
    ret

section .bss
_stack_stash: resq 1

BITS 32

section .text

_call32_return_32:
    add esp, 4
    retf

_call_32_from_32:
    mov eax, 4
    mov ebx, 1
    mov ecx, edi
    mov edx, 44
    int 0x80
    ret
