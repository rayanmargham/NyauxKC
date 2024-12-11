#include "arch.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/instructions/instructions.h"

#include "arch/x86_64/idt/idt.h"
#include "mem/vmm.h"
#include "term/term.h"
#include <arch/x86_64/idt/idt.h>
#include <mem/kmem.h>
#include <stdint.h>
#include <utils/basic.h>
// case 1:
//     outb(address, in_value);
//     break;
//   case 2:
//     outw(address, in_value);
//     break;
//   case 4:
//     outd(address, in_value);
//     break;
//   case 8:
//     return UACPI_STATUS_INVALID_ARGUMENT;
//     break;
//   }
void raw_io_write(uint64_t address, uint64_t data, uint8_t byte_width) {
  switch (byte_width) {
  case 1:
#if defined(__x86_64__)
    outb((uint16_t)address, (uint8_t)data);
#endif
    break;
  case 2:
#if defined(__x86_64__)
    outw((uint16_t)address, (uint16_t)data);
#endif
    break;
  case 4:
#if defined(__x86_64__)
    outd((uint16_t)address, (uint32_t)data);
#endif
    break;
  default:
    break;
  }
}
uint64_t raw_io_in(uint64_t address, uint8_t byte_width) {
  switch (byte_width) {
  case 1:
#if defined(__x86_64__)
    return (uint64_t)inb((uint16_t)address);
#endif
    break;
  case 2:
#if defined(__x86_64__)
    return (uint64_t)inw((uint16_t)address);
#endif
    break;
  case 4:
#if defined(__x86_64__)
    return (uint64_t)ind((uint16_t)address);
#endif
    break;
  default:
    break;
  }
}
int uacpi_arch_install_irq(uacpi_interrupt_handler handler, uacpi_handle ctx,
                           uacpi_handle *out_irq_handle) {
#if defined(__x86_64__)
  // CHANGE WHEN WE DO SMP
  int vec = AllocateIrq();
  if (vec == -1) {
    return -1;
  }
  uacpi_irq_wrap_info *info = kmalloc(sizeof(uacpi_irq_wrap_info));
  info->fn = handler;
  info->ctx = ctx;
  *out_irq_handle = (uacpi_handle)(uint64_t)vec;
  isr_ctxt[vec] = info;
  RegisterHandler(vec, uacpi_wrap_irq_fn);
  // when u get ioapic install the irq pls
#endif
}
void arch_init() {
#if defined(__x86_64__)
  kprintf("Welcome to Nyaux on x86_64!\n");
  init_gdt();
  kprintf("arch_init(): gdt loaded.\n");
  init_idt();
  kprintf("arch_init(): idt loaded.\n");
#else
  kprintf("Nyaux Cannot Run on this archiecture.");
  hcf();
#endif
}
void arch_late_init() {
#if defined(__x86_64__)

  init_gdt();
  init_idt();
  per_cpu_vmm_init();
  kprintf("arch_late_init(): CPU %d is \e[0;32mOnline\e[0;37m!\n",
          get_lapic_id());
  init_lapic();
#endif
}
