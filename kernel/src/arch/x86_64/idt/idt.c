#include "idt.h"

#include <elf/symbols/symbols.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>

#include "../cpu/lapic.h"
#include "../instructions/instructions.h"
#include "arch/arch.h"
#include "arch/x86_64/cpu/structures.h"
#include "mem/vmm.h"
#include "sched/sched.h"

#define INTERRUPT_GATE 0xE
#define TRAP_GATE 0xF
extern uint64_t stubs[];
typedef struct {
  uint16_t offset_1;
  uint16_t segment_selector;
  uint8_t ist_plus_reversed;
  uint8_t type_attributes; // gate type dpl and the present bit
  uint16_t offset_2;
  uint32_t offset_3;
  uint32_t zero;
} __attribute__((packed)) InterruptDescriptor;

typedef struct {
  uint16_t size;
  uint64_t offset;
} __attribute__((packed)) IDTR;
#define STACKTRACE                                                             \
  nyauxsymbol h = find_from_rip(frame->rip);                                   \
  uint64_t *base_ptr = 0;                                                      \
  uint64_t temp = 0;                                                           \
  temp = frame->rbp;                                                           \
  kprintf_symbol(h, frame->rip);                                                           \
  base_ptr = (uint64_t *)temp;                                                 \
  while (base_ptr != 0) {                                                      \
    uint64_t *next = (uint64_t *)*(uint64_t *)(base_ptr);                      \
    if (!next) {                                                               \
      break;                                                                   \
    }                                                                          \
    uint64_t ret_addr = *(uint64_t *)((uint64_t)base_ptr + 8);                 \
    if (ret_addr != 0) {                                                       \
      h = find_from_rip(ret_addr);                                             \
      kprintf_symbol(h, ret_addr);                                                       \
    } else {                                                                   \
      kprintf("-> Function: none -- 0x0\r\n");                                 \
    }                                                                          \
                                                                               \
    base_ptr = (uint64_t *)*(uint64_t *)(base_ptr);                            \
  }

IDTR idr;
InterruptDescriptor IDT[256];
void *(*idt_handlers[256])(struct StackFrame *frame);
// CHANGE THIS WHEN SMP OR FUCKING DIE TODO TODO
void *isr_ctxt[256]; // anyone can store whatever here
void set_gate(int idx, uint16_t segment_selector, uint8_t gate_type,
              uint8_t dpl, uint64_t entry) {
  IDT[idx].segment_selector = segment_selector;
  IDT[idx].offset_1 = entry & 0xFFFF;
  IDT[idx].offset_2 = (entry >> 16) & 0xFFFF;
  IDT[idx].offset_3 = (entry >> 32) & 0xFFFFFFFF;
  IDT[idx].ist_plus_reversed = 0;
  IDT[idx].type_attributes = (gate_type & 0xF) | ((dpl & 0x3) << 5) | (1 << 7);
}
void kernel_interrupt_gate(int idx, uint64_t entry) {
  set_gate(idx, 0x28, INTERRUPT_GATE, 0, entry);
}
extern void idt_flush(void *);

void *kprintf_symbol(nyauxsymbol h, uint64_t rip) {
  kprintf("-> Function: %s() -- 0x%lx+0x%lx\r\n", h.function_name,
          h.function_address, rip - h.function_address);
  return 0;
}

void *uacpi_wrap_irq_fn(struct StackFrame *frame) {
  kprintf("attempting to execute a uacpi irq method\r\n");
  if (isr_ctxt[frame->intnum] == NULL) {
    panic("Could not handle uacpi interrupt :c");
  }
  uacpi_irq_wrap_info *info = isr_ctxt[frame->intnum];
  info->fn(info->ctx);
  return frame;
}
void *division_by_zero(struct StackFrame *frame) {
  kprintf("Division Error\r\n");
  if (symbolarray != NULL) {
    STACKTRACE
  } else {
    kprintf("null im afraid\r\n");
  }
  panic("Error of ze division :c");
  return 0;
}
uint64_t read_cr2() {
  uint64_t cr2 = 0;
  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  return cr2;
}

void *page_fault_handler(struct StackFrame *frame) {
  __asm__ volatile("cli");
  if (arch_get_per_cpu_data()->cur_thread == NULL ||
      arch_get_per_cpu_data()->cur_thread->proc == NULL) {

    kprintf("Page Fault! CR2 0x%lx\r\n", read_cr2());
    kprintf("RIP is 0x%lx. Error Code 0b%B\r\n", frame->rip,
            frame->error_code);
    STACKTRACE
    panic("Page Fault:c");
  }
  struct process_t *proc = arch_get_per_cpu_data()->cur_thread->proc;
  uint64_t virt = read_cr2();
  if (iswithinvmmregion(proc->cur_map, virt)) {
    arch_map_usersingularpage(proc->cur_map, virt);
    return frame;
  }
  if (arch_get_per_cpu_data() != NULL &&
      arch_get_per_cpu_data()->cur_thread != NULL) {
    kprintf("Page Fault! CR2 0x%lx\r\n", read_cr2());
    kprintf("RIP is 0x%lx. Error Code 0x%lx\r\n", frame->rip,
            frame->error_code);
    kprintf("Page Fault Happened in thread %lu\r\n",
            arch_get_per_cpu_data()->cur_thread->tid);
  }
  STACKTRACE
  panic("Page Fault:c");
  return 0;
}
void *general_protection_fault_handler(struct StackFrame *frame) {
  kprintf("General Protection Fault with error code 0x%lx\r\nCR2 0x%lx\r\n",
          frame->error_code, read_cr2());
  STACKTRACE
  panic("fuck\r\n");
  return 0;
}
void *default_handler(struct StackFrame *frame) {
  kprintf("Unhandled interrupt/exception number 0x%lx\r\n", frame->intnum);
  kprintf("CS:RIP is 0x%02lx:0x%lx\r\n", frame->cs, frame->rip);
  STACKTRACE
  panic("CPU halted");
  return 0;
}
void RegisterHandler(int interrupt,
                     void *(*handler)(struct StackFrame *frame)) {
  idt_handlers[interrupt] = handler;
}
// 32 is reversed for the scheduler
int AllocateIrq() {
  for (int i = 33; i < 256; i++) {
    if (idt_handlers[i] == default_handler) {
      kprintf("AllocateIrq(): Found irq vector %d\r\n", i);
      return i;
    }
  }
  return -1;
}
void *sched(struct StackFrame *frame) {
  __asm__ volatile("cli");
  unsigned long rsp;

  // Inline assembly to read the RSP register
  __asm__ __volatile__("mov %%rsp, %0"
                       : "=r"(rsp) // Output operand
  );

  schedd(frame);
  send_eoi();
  return frame;
}
void init_idt() {
  for (int i = 0; i < 256; i++) {
    // Register a default handler
    RegisterHandler(i, default_handler);

    // Setup an IDT entry for all the interrupt stubs
    kernel_interrupt_gate(i, stubs[i]);
    isr_ctxt[i] = NULL;
  };
  RegisterHandler(0, division_by_zero);
  RegisterHandler(0xe, page_fault_handler);
  RegisterHandler(0xd, general_protection_fault_handler);
  RegisterHandler(32, sched);
  idr.offset = (uint64_t)&IDT;
  idr.size = sizeof(IDT) - 1;

  idt_flush(&idr);
}
void per_cpu_init_idt() { idt_flush(&idr); }
