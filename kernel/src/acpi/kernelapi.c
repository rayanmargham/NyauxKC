#include "kernelapi.h"

#include <arch/arch.h>
#include <stdint.h>

#include "mem/pmm.h"
#include "timers/hpet.h"
#include "uacpi/status.h"
#include "uacpi/types.h"
#include "utils/basic.h"

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* out_rdsp_address)
{
	*out_rdsp_address = (uint64_t)rsdp_request.response->address - hhdm_request.response->offset;
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64* out_value)
{
	uint64_t virt = (uint64_t)address + hhdm_request.response->offset;
	switch (byte_width)
	{
		case 1: *out_value = *(uint8_t*)virt; break;
		case 2: *out_value = *(uint16_t*)virt; break;
		case 4: *out_value = *(uint32_t*)virt; break;
		case 8: *out_value = *(uint64_t*)virt; break;
		default: return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 in_value)
{
	uint64_t* virt = (uint64_t*)((uint64_t)address + hhdm_request.response->offset);
	switch (byte_width)
	{
		case 1: *(uint8_t*)virt = in_value; break;
		case 2: *(uint16_t*)virt = in_value; break;
		case 4: *(uint32_t*)virt = in_value; break;
		case 8: *(uint64_t*)virt = in_value; break;
		default: return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64* out_value)
{
	*out_value = raw_io_in(address, byte_width);
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64 in_value)
{
	raw_io_write(address, in_value, byte_width);
	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read(uacpi_pci_address* address, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64* value)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_pci_write(uacpi_pci_address* address, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 value)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
struct io_range
{
	uacpi_io_addr base;
	uacpi_size len;
};
uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle* out_handle)
{
	struct io_range* rng = kmalloc(sizeof(struct io_range));
	rng->base = base;
	rng->len = len;
	*out_handle = (uacpi_handle)rng;
	return UACPI_STATUS_OK;
}
void uacpi_kernel_io_unmap(uacpi_handle handle)
{
	kfree((void*)(struct io_range*)handle, sizeof(struct io_range));
}
uacpi_status uacpi_kernel_io_read(uacpi_handle hnd, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64* value)
{
	struct io_range* rng = (struct io_range*)hnd;
	if (offset >= rng->len)
	{
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return uacpi_kernel_raw_io_read(rng->base + offset, byte_width, value);
}
uacpi_status uacpi_kernel_io_write(uacpi_handle hnd, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 value)
{
	struct io_range* rng = (struct io_range*)hnd;
	if (offset >= rng->len)
	{
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return uacpi_kernel_raw_io_write(rng->base + offset, byte_width, value);
}
void* uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
	return (void*)((uint64_t)addr + hhdm_request.response->offset);
}
void uacpi_kernel_unmap(void* addr, uacpi_size len)
{
	// nothing
}
void* uacpi_kernel_alloc(uacpi_size size)
{
	return kmalloc(size);
}
void* uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
	void* man = kmalloc(count * size);
	return man;
}
void uacpi_kernel_free(void* mem, uacpi_size size_hint)
{
	kfree(mem, size_hint);
}
void uacpi_kernel_log(uacpi_log_level log, const uacpi_char* cha)
{
	kprintf("uacpi: %s", (char*)cha);
}
uacpi_u64 uacpi_kernel_get_ticks(void)
{
	return 46644;
}

void uacpi_kernel_stall(uacpi_u8 usec)
{
}
void uacpi_kernel_sleep(uacpi_u64 msec)
{
}
uacpi_handle uacpi_kernel_create_mutex(void)
{
	spinlock_t* mutex = kmalloc(sizeof(spinlock_t));
	return mutex;
}
void uacpi_kernel_free_mutex(uacpi_handle f)
{
	kfree((void*)f, sizeof(spinlock_t));
}
void uacpi_kernel_free_spinlock(uacpi_handle f)
{
	kfree((void*)f, sizeof(spinlock_t));
}
uacpi_handle uacpi_kernel_create_event(void)
{
	return (void*)1;
}

void uacpi_kernel_free_event(uacpi_handle)
{
}

uacpi_thread_id uacpi_kernel_get_thread_id(void)
{
	return 0;
}
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle thing, uacpi_u16 w)
{
	spinlock_lock((spinlock_t*)thing);
	return UACPI_STATUS_OK;
}
void uacpi_kernel_release_mutex(uacpi_handle thing)
{
	spinlock_unlock((spinlock_t*)thing);
}
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16)
{
	return true;
}
void uacpi_kernel_signal_event(uacpi_handle)
{
}
void uacpi_kernel_reset_event(uacpi_handle)
{
}
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*)
{
	return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
													uacpi_handle* out_irq_handle)
{
	int ret = uacpi_arch_install_irq(irq, handler, ctx, out_irq_handle);
	if (ret == -1)
	{
		return UACPI_STATUS_NO_HANDLER;
	}

	return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle irq_handle)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_handle uacpi_kernel_create_spinlock(void)
{
	spinlock_t* mutex = kmalloc(sizeof(spinlock_t));
	return mutex;
}
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle lock)
{
	__asm__ volatile("cli");
	spinlock_lock((spinlock_t*)lock);
	return 1;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle lock, uacpi_cpu_flags)
{
	spinlock_unlock((spinlock_t*)lock);
	__asm__ volatile("sti");
}
uacpi_status uacpi_kernel_schedule_work(uacpi_work_type, uacpi_work_handler, uacpi_handle ctx)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_status uacpi_kernel_wait_for_work_completion(void)
{
	return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void)
{
	return read_hpet_counter();
}
