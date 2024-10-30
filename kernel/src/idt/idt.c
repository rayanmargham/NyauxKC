#include "idt.h"
#include "term/term.h"
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

IDTR idr;
InterruptDescriptor IDT[256];
void (*idt_handlers[256])(struct StackFrame *frame);
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
void RegisterHandler(int interrupt, void (*handler)(struct StackFrame *frame)) {
  idt_handlers[interrupt] = handler;
}
void division_by_zero(struct StackFrame *frame) {
  kprintf("Amazing\n");
  kprintf("Look at this cool int num %lx\n", frame->intnum);
  panic("yooo lets goooo");
}
uint64_t read_cr2() {
  uint64_t cr2 = 1;
  __asm__ volatile("mov %0, %%cr2" : "=r"(cr2));
  return cr2;
}
void page_fault_handler(struct StackFrame *frame) {
  kprintf("Page Fault! CR2 0x%lx\n", read_cr2());
  kprintf("RIP is 0x%lx\n", frame->rip);
  panic("Page Fault:c");
}

void init_idt() {
  for (int i = 0; i < 256; i++) {
    kernel_interrupt_gate(i, stubs[i]);
  };
  RegisterHandler(0, division_by_zero);
  RegisterHandler(0xe, page_fault_handler);
  idr.offset = (uint64_t)&IDT;
  idr.size = sizeof(IDT) - 1;

  idt_flush(&idr);
}