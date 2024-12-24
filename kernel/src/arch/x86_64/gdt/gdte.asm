global gdt_flush
extern change_rsp0
global do_savekstackandloadkstack
gdt_flush:
    lgdt [rdi]
    mov eax, 0x48
    ltr ax
    push 0x28
    lea rax, [rel .reload_CS]
    push rax
    retfq
.reload_CS:
    mov ax, 0x30
    mov ss, ax
    ret
global return_from_kernel_in_new_thread
return_from_kernel_in_new_thread:
    add rsp, 8 ; skip int num
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

    test byte [rsp + 16], 0x3; Are lower 2 bits(priv lvl) of CS selector 0?
    jz .skipswapgs3          ; If they are 0 (kernel mode) then skip swap
    swapgs                   ; Otherwise swapgs in user mode
.skipswapgs3:
    add rsp, 8               ; Skip error code
    iretq
do_savekstackandloadkstack: ; rdi has thread old and rsi has thread new
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15 ; dont ask why these 6 registers only, mr miskakov never bothered explaining
    test rdi, rdi
    jz .skipstore
    mov [rdi + 0xB8], rsp ; store into kernel stack ptr the value of rsp
    .skipstore
    mov rsp, [rsi + 0xB8] ; load from kernel stack ptr into rsp
    mov rdi, [rsi + 0xC0] ; ;pad kernel stack base in rsp0
    call change_rsp0 
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret