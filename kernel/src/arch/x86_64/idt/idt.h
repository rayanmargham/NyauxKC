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
#define STACKTRACE                                                             \
  nyauxsymbol h = find_from_rip(frame->rip);                                   \
  uint64_t *base_ptr = 0;                                                      \
  uint64_t temp = 0;                                                           \
  temp = frame->rbp;                                                           \
  kprintf_symbol(h, frame->rip);                                               \
  base_ptr = (uint64_t *)temp;                                                 \
  while (base_ptr != 0) {                                                      \
    uint64_t *next = (uint64_t *)*(uint64_t *)(base_ptr);                      \
    if (!next) {                                                               \
      break;                                                                   \
    }                                                                          \
    uint64_t ret_addr = *(uint64_t *)((uint64_t)base_ptr + 8);                 \
                                                                       \
    if (ret_addr != 0 && ret_addr >= hhdm_request.response->offset) {                                                       \
      h = find_from_rip(ret_addr);                                             \
      kprintf_symbol(h, ret_addr);                                             \
    } else if (ret_addr < hhdm_request.response->offset) {                                                                   \
      kprintf("-> Function: UserSpace Function -- 0x%lx\r\n", ret_addr);                                 \
    }else {\
      kprintf("-> Function: none -- 0x0\r\n");\
    }                      \
    \
    base_ptr = (uint64_t *)*(uint64_t *)(base_ptr);                            \
  }

#ifdef __cplusplus
}
#endif