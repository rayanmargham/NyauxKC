#pragma once
#include "timers/pvclock.hpp"
#ifdef __cplusplus
extern "C" {
#endif
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
    uint64_t fs_base;
    // uhhhh lol
  };
  struct arch_per_cpu_data {
    uint64_t syscall_stack_ptr_tmp;
    uint64_t
        kernel_stack_ptr; // this has to get updated when switching threads.
                          // its just indirection, so its easier for the cpu
                          // to get the kernel stack for syscall
    // other things like lapic id etcetc
    uint32_t lapic_id;
    struct TSS tss;
    // yea so this may be a biiiiiitt cursed but listen fuck you okay im tired
    // as balls and this seems like the easiest option this will be NULL when
    // there is no pvclock
    struct pvclock_vcpu_time_info *pvclock;
  };
#define x86_KERNEL_GS_BASE 0xC0000101
#define x86_FS_BASE 0xC0000100
#ifdef __cplusplus
}
#endif
