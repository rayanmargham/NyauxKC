section .data
msg  db  'Hello, world!',0xa
len equ $ - msg
section .text
global shitfuck
shitfuck:
    mov rax, 1
    mov rdi, msg
    mov rsi, len
    syscall
    jmp $
    