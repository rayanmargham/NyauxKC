extern syscall_exit
extern syscall_debug
global syscall_entry
section .data
syscallarray:
    dq syscall_exit ; 0
    dq syscall_debug ; 1
.length: dq ($ - syscallarray) / 8
section .text
syscall_entry:
    swapgs
    mov [gs:0], rsp
    mov rsp, [gs:8]
    push rcx
    push rdx
    push rbp
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


    mov rcx, r8 ; move argument #4 from r8 to rcx
    mov r8, r9  ; move argument #5 from r9 to r8
    mov r9, r10 ; move argument #6 from r10 to r9
    mov rax, [syscallarray + rax * 8]
    call rax
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
    pop rbp
    pop rdx
    pop rcx
    
    mov rsp, [gs:0]
    swapgs
    o64 sysret


