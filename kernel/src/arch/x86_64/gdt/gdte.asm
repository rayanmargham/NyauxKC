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
