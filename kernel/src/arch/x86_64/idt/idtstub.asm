extern idt_handlers

section .text

global idt_flush
idt_flush:
    lidt [rdi]
    ret

; ISR stub generation macro
; 1st Argument is the ISR number
; 2nd Argument optional error code to push
%macro isr_stub 1-2
global isr_stub_%1
isr_stub_%1:
    ; Push a dummy error code if a 2nd parameter has been passed to the macro
%if %0 == 2
    push %2                  ; Push dummy error code
%endif
    test byte [rsp + 16], 0x3; Are lower 2 bits(priv lvl) of CS selector 0?
    jz .skipswapgs1          ; If they are 0 (kernel mode) then skip swap
    swapgs                   ; Otherwise swapgs in user mode
.skipswapgs1:
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

    push gs                  ; Save previous state of segment registers
    push fs


    push %1                  ; Push the interrupt number

    mov rdi, rsp ; put value of stack pointer into paramter 1 of c interrupt handler
    mov rax, [idt_handlers + %1 * 8]
                             ; Get the registered handler to call
    cld                      ; Required by the 64-bit System V ABI
    call rax
    mov rsp, rax
    add rsp, 8               ; skip int number
              ; Restore previous state of segment registers
    pop fs
    pop gs

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
    jz .skipswapgs2          ; If they are 0 (kernel mode) then skip swap
    swapgs                   ; Otherwise swapgs in user mode
.skipswapgs2:
    add rsp, 8               ; Skip error code
    iretq
%endmacro

; Define stubs for exceptions (interrupts 0-31)
; Second parameter to isr_stub is the dummy error code (0) to push
;     for interrupts that the CPU doesn't push an error code for
isr_stub 0, 0                  ; Divide by Zero
isr_stub 1, 0                  ; Debug Exception
isr_stub 2, 0                  ; NMI Interrupt
isr_stub 3, 0                  ; Breakpoint
isr_stub 4, 0                  ; Overflow
isr_stub 5, 0                  ; Out of Bounds
isr_stub 6, 0                  ; Invalid Opcode
isr_stub 7, 0                  ; Device Not Available
isr_stub 8                     ; Double Fault
isr_stub 9, 0                  ; Coprocessor Segment Overrun
isr_stub 10                    ; Invalid TSS
isr_stub 11                    ; Segment Not Present
isr_stub 12                    ; Stack Fault
isr_stub 13                    ; General Protection Fault
isr_stub 14                    ; Page Fault
isr_stub 15, 0                 ; Reserved
isr_stub 16, 0                 ; x87 Floating Point Exception
isr_stub 17                    ; Alignment Check
isr_stub 18, 0                 ; Machine Check
isr_stub 19, 0                 ; SIMD Floating Point Exception
isr_stub 20, 0                 ; Reserved
isr_stub 21                    ; Reserved
isr_stub 22, 0                 ; Reserved
isr_stub 23, 0                 ; Reserved
isr_stub 24, 0                 ; Reserved
isr_stub 25, 0                 ; Reserved
isr_stub 26, 0                 ; Reserved
isr_stub 27, 0                 ; Reserved
isr_stub 28, 0                 ; Reserved
isr_stub 29                    ; Reserved
isr_stub 30                    ; Security Exception
isr_stub 31, 0                 ; Reserved
isr_stub 32, 0
; All external interrupts (32-47) are the treated
; the same as an ISR without an error code pushed
; by the CPU.

%assign cur_int 33
%rep 224
isr_stub cur_int, 0
%assign cur_int cur_int+1
%endrep

section .rodata

; Emit a stub entry
%macro emit_stub 1
    dq isr_stub_%1
%endmacro

; Emit the stub table
global stubs
stubs:
%assign cur_int 0
%rep 256
    emit_stub cur_int
%assign cur_int cur_int+1
%endrep
