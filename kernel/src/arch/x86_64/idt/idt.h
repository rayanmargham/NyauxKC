#pragma once
#include <elf/symbols/symbols.h>
#include <stdint.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <utils/basic.h>
#include "../cpu/structures.h"
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
