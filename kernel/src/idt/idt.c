#include "idt.h"
#include "elf/symbols/symbols.h"
#include "term/term.h"
#include "timers/lapic.h"
#include "utils/basic.h"

#include <stdint.h>
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
  kprintf_symbol(h);                                                           \
  base_ptr = (uint64_t *)temp;                                                 \
  while (base_ptr != 0) {                                                      \
    uint64_t ret_addr = *(uint64_t *)((uint64_t)base_ptr + 8);                 \
    if (ret_addr != 0) {                                                       \
      h = find_from_rip(ret_addr);                                             \
      kprintf_symbol(h);                                                       \
    } else {                                                                   \
      kprintf("-> Function: none -- 0x0\n");                                   \
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

void *kprintf_symbol(nyauxsymbol h) {
  kprintf("-> Function: %s() -- 0x%lx\n", h.function_name, h.function_address);
  return 0;
}

void *division_by_zero(struct StackFrame *frame) {
  kprintf("Division Error\n");
  if (symbolarray != NULL) {
    STACKTRACE
  } else {
    kprintf("null im afraid\n");
  }
  panic("Error of ze division :c");
  return 0;
}
uint64_t read_cr2() {
  uint64_t cr2 = 1;
  __asm__ volatile("mov %0, %%cr2" : "=r"(cr2));
  return cr2;
}

void *page_fault_handler(struct StackFrame *frame) {
  kprintf("Page Fault! CR2 0x%lx\n", read_cr2());
  kprintf("RIP is 0x%lx\n", frame->rip);
  STACKTRACE
  panic("Page Fault:c");
  return 0;
}

void *default_handler(struct StackFrame *frame) {
  kprintf("Unhandled interrupt/exception number 0x%x\n", frame->intnum);
  kprintf("CS:RIP is 0x%02x:0x%lx\n", frame->cs, frame->rip);
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
      kprintf("AllocateIrq(): Found irq vector %d\n", i);
      return i;
    }
  }
  return -1;
}
void *sched(struct StackFrame *frame) {
  if (get_lapic_id() == 0) {
    send_eoi();
    return frame;
  }
  kprintf("CPU %d Says: MY LAPIC TICKED HAHAHAHA\n", get_lapic_id());
  send_eoi();
  return frame;
}
void init_idt() {
  for (int i = 0; i < 256; i++) {
    // Register a default handler
    RegisterHandler(i, default_handler);

    // Setup an IDT entry for all the interrupt stubs
    kernel_interrupt_gate(i, stubs[i]);
  };
  RegisterHandler(0, division_by_zero);
  RegisterHandler(0xe, page_fault_handler);
  RegisterHandler(32, sched);
  idr.offset = (uint64_t)&IDT;
  idr.size = sizeof(IDT) - 1;

  idt_flush(&idr);
}
