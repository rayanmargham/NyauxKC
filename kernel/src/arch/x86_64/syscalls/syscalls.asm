extern syscall_exit
extern syscall_debug
extern syscall_setfsbase
extern syscall_mmap
extern syscall_openat
extern syscall_read
extern syscall_seek
extern syscall_close
extern syscall_isatty
extern syscall_write
extern syscall_ioctl
extern syscall_dup
extern syscall_fstat
extern syscall_getcwd
extern syscall_fork
extern syscall_waitpid
extern syscall_getpid
extern syscall_free
global syscall_entry
section .data
syscallarray:
    dq syscall_exit ; 0
    dq syscall_debug ; 1
    dq syscall_setfsbase ; 2
    dq syscall_mmap ; 3
    dq syscall_openat ; 4
    dq syscall_read ; 5
    dq syscall_seek ; 6
    dq syscall_close ; 7
    dq syscall_isatty ; 8
    dq syscall_write ; 9
    dq syscall_ioctl ; 10
    dq syscall_dup ; 11
    dq syscall_fstat ; 12
    dq syscall_getcwd ; 13
    dq syscall_fork ; 14
    dq syscall_waitpid ; 15
    dq syscall_getpid ; 16
    dq syscall_free ; 17

.length: dq ($ - syscallarray) / 8
section .text
syscall_entry:
    swapgs
    mov [gs:0], rsp
    mov rsp, [gs:8]
    push rax ; pad to 16 bytes
    push rbx
    push rcx
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
    pop rcx
    pop rbx
    ; pad gets discarded by the below move to rsp

    mov rsp, [gs:0]
    swapgs
    o64 sysret


