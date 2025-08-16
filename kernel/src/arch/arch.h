#pragma once

#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>

#include "mem/vmm.h"
#include <uacpi/types.h>
#if defined(__x86_64__)
#include "x86_64/cpu/structures.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
void arch_raw_io_write(uint64_t address, uint64_t data, uint8_t byte_width);
uint64_t arch_raw_io_in(uint64_t address, uint8_t byte_width);
int uacpi_arch_install_irq(uacpi_u32 irq, uacpi_interrupt_handler handler,
                           uacpi_handle ctx, uacpi_handle *out_irq_handle);
void arch_init();
void arch_late_init();
uint64_t arch_mapkernelhhdmandmemorymap(pagemap *take);
void arch_map_vmm_region(pagemap *take, uint64_t base, uint64_t length_in_bytes,
                         bool user);
bool arch_check_can_pvclock();
void arch_unmap_vmm_region(pagemap *take, uint64_t base,
                           uint64_t length_in_bytes);
uint64_t arch_get_phys(pagemap *take, uint64_t virt);
void arch_init_pagemap(pagemap *take);
uint64_t arch_completeinit_pagemap(pagemap *take);
void arch_destroy_pagemap(pagemap *take);
void arch_switch_pagemap(pagemap *take);
void arch_init_interruptcontrollers();
void arch_enable_interrupts();
void arch_map_usersingularpage(pagemap *take, uint64_t virt);
void arch_disable_interrupts();
bool arch_check_kvm_clock();
bool arch_is_mapped_buf(pagemap *take, uint64_t virt, size_t size);
bool arch_is_mapped(pagemap *take, uint64_t virt);
#ifdef __x86_64__
#define ARCH_CHECK_SPACE(amount) (align_up((amount), 4096) + 0x1000)
struct StackFrame arch_create_frame(bool usermode, uint64_t entry_func,
                                    uint64_t stack);
#endif
#ifdef __cplusplus
}
#endif
