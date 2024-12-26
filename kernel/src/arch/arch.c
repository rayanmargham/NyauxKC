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
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
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
void raw_io_write(uint64_t address, uint64_t data, uint8_t byte_width)
{
	switch (byte_width)
	{
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
		default: break;
	}
}
uint64_t raw_io_in(uint64_t address, uint8_t byte_width)
{
	switch (byte_width)
	{
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
		default: panic("raw_io_in(): not a valid byte width"); break;
	}
}
int uacpi_arch_install_irq(uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle* out_irq_handle)
{
#if defined(__x86_64__)
	// CHANGE WHEN WE DO SMP
	int vec = AllocateIrq();
	if (vec == -1)
	{
		kprintf("true\n");
		return -1;
	}
	uacpi_irq_wrap_info* info = kmalloc(sizeof(uacpi_irq_wrap_info));
	info->fn = handler;
	info->ctx = ctx;
	*out_irq_handle = (uacpi_handle)(uint64_t)vec;
	isr_ctxt[vec] = info;
	RegisterHandler(vec, uacpi_wrap_irq_fn);
	// when u get ioapic install the irq pls
	return vec;
#endif
}
void arch_init()
{
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
void arch_late_init()
{
#if defined(__x86_64__)
	per_cpu_vmm_init();
	arch_create_per_cpu_data();
	init_gdt();
	init_idt();

	kprintf("arch_late_init(): CPU %d is \e[0;32mOnline\e[0;37m!\n", get_lapic_id());
	init_lapic();
#endif
}
uint64_t arch_mapkernelhhdmandmemorymap(pagemap* take)
{
#if defined(__x86_64__)
	return x86_64_map_kernelhhdmandmemorymap(take);
#endif
}
void arch_map_vmm_region(pagemap* take, uint64_t base, uint64_t length_in_bytes)
{
#if defined(__x86_64__)
	return x86_64_map_vmm_region(take, base, length_in_bytes);
#endif
}
void arch_unmap_vmm_region(pagemap* take, uint64_t base, uint64_t length_in_bytes)
{
#if defined(__x86_64__)
	return x86_64_unmap_vmm_region(take, base, length_in_bytes);
#endif
}
void arch_init_pagemap(pagemap* take)
{
#if defined(__x86_64__)
	x86_64_init_pagemap(take);
#endif
}
void arch_destroy_pagemap(pagemap* take)
{
#if defined(__x86_64__)
	x86_64_destroy_pagemap(take);
#endif
}
void arch_switch_pagemap(pagemap* take)
{
#if defined(__x86_64__)
	x86_64_switch_pagemap(take);
#endif
}
#if defined(__x86_64__)
struct StackFrame arch_create_frame(bool usermode, uint64_t entry_func, uint64_t stack)
{
	if (usermode)
	{
		struct StackFrame meow = {.rip = entry_func,
								  .rsp = stack,
								  .cs = 0x40 | (3),	   // USER CODE
								  .ss = 0x38 | (3),	   // USER DATA
								  .rbp = stack,
								  .rflags = 0x202};
		return meow;
	}
	else
	{
		struct StackFrame meow = {.rip = entry_func,
								  .rsp = stack,
								  .cs = 0x28,	 // USER CODE
								  .ss = 0x30,	 // USER DATA
								  .rbp = stack,
								  .rflags = 0x202};
		return meow;
	}
}
#endif
void arch_init_interruptcontrollers()
{
#ifdef __x86_64__
	populate_ioapic();
#endif
}
