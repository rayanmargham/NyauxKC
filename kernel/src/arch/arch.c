#include "arch.h"

#include <arch/x86_64/idt/idt.h>
#include <mem/kmem.h>
#include <stdint.h>
#include <utils/basic.h>

#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/cpu/structures.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/instructions/instructions.h"
#include "arch/x86_64/interrupt_controllers/ioapic.h"
#include "arch/x86_64/page_tables/pt.h"
#include "arch/x86_64/syscalls/syscall.h"
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include "x86_64/fpu/xsave.h"
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
void arch_raw_io_write(uint64_t address, uint64_t data, uint8_t byte_width) {
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
uint64_t arch_raw_io_in(uint64_t address, uint8_t byte_width) {
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
    return (uint64_t)ind((uint32_t)address);
#endif
    break;
  default:
    panic("raw_io_in(): not a valid byte width");
    break;
  }
}
uint64_t arch_get_phys(pagemap *take, uint64_t virt) {
#if defined(__x86_64__)
  return x86_64_get_phys(take, virt);
#endif
}
bool arch_is_mapped(pagemap *take, uint64_t virt) {
  #if defined(__x86_64__)

  return x86_64_is_mapped(take, virt);
  #endif
}
bool arch_is_mapped_buf(pagemap *take, uint64_t virt, size_t size) {
  #if defined(__x86_64__)
  return x86_64_is_mapped_buf(take, virt, size);
  #endif
}
int uacpi_arch_install_irq(uacpi_u32 irq, uacpi_interrupt_handler handler,
                           uacpi_handle ctx, uacpi_handle *out_irq_handle) {
#if defined(__x86_64__)
  // CHANGE WHEN WE DO SMP
  int vec = AllocateIrq();
  if (vec == -1) {
    kprintf("true\r\n");
    return -1;
  }
  uacpi_irq_wrap_info *info = kmalloc(sizeof(uacpi_irq_wrap_info));
  info->fn = handler;
  info->ctx = ctx;
  *out_irq_handle = (uacpi_handle)(uint64_t)vec;
  isr_ctxt[vec] = info;
  RegisterHandler(vec, uacpi_wrap_irq_fn);
  // when u get ioapic install the irq pls
  route_irq(irq, vec, 0, get_lapic_id());
  return vec;
#endif
}
void arch_init() {
#if defined(__x86_64__)
  kprintf("Welcome to Nyaux on x86_64!\r\n");
  arch_create_bsp_per_cpu_data();
  init_gdt();
  init_idt();
  fpu_init();
  syscall_init();

#else
#error cooked
#endif
}
void arch_late_init() {
#if defined(__x86_64__)
  per_cpu_vmm_init();
  arch_create_per_cpu_data();
  init_gdt();
  per_cpu_init_idt();
  fpu_init();
  syscall_init();

  kprintf("arch_late_init(): CPU %d is \e[0;32mOnline\e[0;37m!\r\n",
          get_lapic_id());
  init_lapic();
#endif
}
void arch_disable_interrupts() {
#if defined(__x86_64__)
  __asm__ volatile("cli");
#endif
}
void arch_enable_interrupts() {
#if defined(__x86_64__)
  __asm__ volatile("sti");
#endif
}
uint64_t arch_mapkernelhhdmandmemorymap(pagemap *take) {
#if defined(__x86_64__)
  return x86_64_map_kernelhhdmandmemorymap(take);
#endif
}
void arch_map_vmm_region(pagemap *take, uint64_t base, uint64_t length_in_bytes,
                         bool user) {
#if defined(__x86_64__)
  if (!user)
    return x86_64_map_vmm_region(take, base, length_in_bytes);
  return x86_64_map_vmm_region_user(take, base, length_in_bytes);
#endif
}
void arch_unmap_vmm_region(pagemap *take, uint64_t base,
                           uint64_t length_in_bytes) {
#if defined(__x86_64__)
  return x86_64_unmap_vmm_region(take, base, length_in_bytes);
#endif
}
uint64_t arch_completeinit_pagemap(pagemap *take) {
#if defined(__x86_64__)
  x86_64_init_pagemap(take);

  return 0;
#endif
}
void arch_init_pagemap(pagemap *take) {
#if defined(__x86_64__)
  x86_64_init_pagemap(take);
#endif
}
void arch_destroy_pagemap(pagemap *take) {
#if defined(__x86_64__)
  x86_64_destroy_pagemap(take);
#endif
}
void arch_switch_pagemap(pagemap *take) {
#if defined(__x86_64__)
  x86_64_switch_pagemap(take);
#endif
}
bool arch_check_kvm_clock() {
#if defined(__x86_64__)
  uint32_t eax, ecx, ebx, edx;
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  if ((uint64_t)ecx & (1ul << 31)) {
    kprintf("arch()(x86_64): running in a HyperVisor, checking if kvm clock is "
            "available\r\n");
    // check if this is KVM

    if (!checkkvmvendor()) {
      return false;
    }
    // we are in kvm lol lesgo
    // check if kvm clock is a feature we have
    cpuid(0x40000001, 0, &eax, &ebx, &ecx, &edx);
    if (eax & (1 << 3)) { // we only want pvclock
      kprintf("arch()(x86_64): kvm clock is available, using kvm clock\r\n");
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
#endif
  return false;
}
#if defined(__x86_64__)
struct StackFrame arch_create_frame(bool usermode, uint64_t entry_func,
                                    uint64_t stack) {
  if (usermode) {
    struct StackFrame meow = {.rip = entry_func,
                              .rsp = stack,
                              .cs = 0x40 | (3), // USER CODE
                              .ss = 0x38 | (3), // USER DATA
                              .rbp = 0,
                              .rflags = 0x202};
    return meow;
  } else {
    struct StackFrame meow = {.rip = entry_func,
                              .rsp = stack,
                              .cs = 0x28, // USER CODE
                              .ss = 0x30, // USER DATA
                              .rbp = 0,
                              .rflags = 0x202};
    return meow;
  }
}
#endif
void arch_init_interruptcontrollers() {
#ifdef __x86_64__
  populate_ioapic();
#endif
}
void arch_map_usersingularpage(pagemap *take, uint64_t virt) {
#ifdef __x86_64__
  x86_64_map_usersingular_page(take, virt);
#endif
}
bool arch_check_can_pvclock() {
#ifdef __x86_64__
  uint32_t eax, ecx, ebx, edx;
  cpuid(0x4000001, 0, &eax, &ebx, &ecx, &edx);
  return eax & (1 << 3);
#endif
}
void arch_enable_kvmpvclock() {
#ifdef __x86_64__

#endif
}
