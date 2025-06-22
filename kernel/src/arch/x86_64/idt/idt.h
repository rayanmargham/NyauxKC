#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <elf/symbols/symbols.h>
#include <stdint.h>
#include <term/term.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <utils/basic.h>

#include "../cpu/structures.h"
int AllocateIrq();
void init_idt();
extern void *isr_ctxt[256];
extern volatile struct limine_hhdm_request
    hhdm_request;
void RegisterHandler(int interrupt, void *(*handler)(struct StackFrame *frame));
typedef struct {
  uacpi_interrupt_handler fn;
  uacpi_handle ctx;
} uacpi_irq_wrap_info;
void *uacpi_wrap_irq_fn(struct StackFrame *frame);
void per_cpu_init_idt();
#ifdef __cplusplus
}
#endif