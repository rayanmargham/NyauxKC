#pragma once
#include <elf/symbols/symbols.h>
#include <stdint.h>
#include <utils/basic.h>
struct StackFrame {
  uint64_t intnum, ds, es, fs, gs;
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rbp, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));
#include <term/term.h>
int AllocateIrq();
void init_idt();
extern void *isr_ctxt[256];
void RegisterHandler(int interrupt, void (*handler)(struct StackFrame *frame));