#pragma once
#include <stdint.h>
#include <utils/basic.h>
struct StackFrame {
  uint64_t intnum, ds, es, fs, gs;
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rbp, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));
#include <term/term.h>
void init_idt();
