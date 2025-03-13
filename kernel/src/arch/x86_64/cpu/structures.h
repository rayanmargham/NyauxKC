#pragma once
#include <stdint.h>

#include "arch/x86_64/gdt/gdt.h"
struct StackFrame {
  uint64_t intnum;
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rbp, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));
struct arch_thread_t {
  struct StackFrame frame;
  // uhhhh lol
};
struct arch_per_cpu_data {
  uint64_t syscall_stack_ptr_tmp;
  uint64_t kernel_stack_ptr; // this has to get updated when switching threads.
                             // its just indirection, so its easier for the cpu
                             // to get the kernel stack for syscall
  // other things like lapic id etcetc
  uint32_t lapic_id;
  struct TSS tss;
};
