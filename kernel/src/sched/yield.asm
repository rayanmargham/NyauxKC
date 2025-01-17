global sched_yield
extern schedd
global load_ctx
global save_ctx
extern memcpy
save_ctx: ; (old frame in rdi, new frame in rsi)
    test rsi, rsi
    je .escape
    mov rdx, 176
    jmp memcpy
    .escape:
    ret
load_ctx:
    mov rsp, rdi
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    add rsp, 8
    iretq
sched_yield:
    ; thanks to @48cf on github for the help in writing this
    ; this was a PAIN to get working, check out some of @48cf's work on git!!! :)
    push rax

    ; push ss
    xor eax, eax
    mov ax, ss
    push rax

    ; push rsp
    lea rax, [rsp+8]
    push rax

    ; push rflags
    pushf

    ; push cs
    xor eax, eax
    mov ax, cs
    push rax

    ; push rip
    mov rax, [rsp+40]
    push rax

    ; push ec
    push 0

    ; push everything else
    push rbp
    mov rax, [rsp+56]
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; push intvec
    push 0
    mov rdi, rsp
    call schedd
    ret