#pragma once
#include <elf/symbols/symbols.h>
#include <stdint.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <utils/basic.h>
struct StackFrame {
  uint64_t intnum, fs, gs;
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rbp, error_code, rip, cs, rflags, rsp, ss;
} __attribute__((packed));
#include <term/term.h>
int AllocateIrq();
void init_idt();
extern void *isr_ctxt[256];
void RegisterHandler(int interrupt, void *(*handler)(struct StackFrame *frame));
typedef struct {
  uacpi_interrupt_handler fn;
  uacpi_handle ctx;
} uacpi_irq_wrap_info;
void *uacpi_wrap_irq_fn(struct StackFrame *frame);
