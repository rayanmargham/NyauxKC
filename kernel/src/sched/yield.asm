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
    push rsp ; old rsp val + 8
    push rax ; + 8
    push qword [rsp+16] ; + 8
    mov ax, ss
    push ax ; + 8
    
    push qword [rsp + 24] ; + 8
    pushf ; + 8
    
    mov ax, cs
    push ax ; + 8
    add rsp, 32
    pop rax
    sub rsp, 24
    push rax ; + 8
    push rbp ; + 8
    add rsp, 40
    pop rax
    sub rsp, 32
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
    push qword 0

    ; push gs                  ; Save previous state of segment registers
    ; push fs
    mov rdi, rsp
    call schedd
    ret