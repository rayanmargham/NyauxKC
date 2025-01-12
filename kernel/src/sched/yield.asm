global sched_yield
extern schedd
sched_yield:
    push rbp
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

    ; push gs                  ; Save previous state of segment registers
    ; push fs
    mov rdi, rsp
    call schedd
   