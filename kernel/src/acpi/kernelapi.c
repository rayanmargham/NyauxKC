#include "kernelapi.h"

#include <arch/arch.h>
#include <stdint.h>

#include "mem/pmm.h"
#include "pci/pci.h"
#include "timers/timer.hpp"
#include "uacpi/kernel_api.h"
#include "uacpi/status.h"
#include "uacpi/types.h"
#include "utils/basic.h"

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rdsp_address) {
  *out_rdsp_address =
      (uint64_t)rsdp_request.response->address - hhdm_request.response->offset;
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address,
                                          uacpi_u8 byte_width,
                                          uacpi_u64 *out_value) {
  uint64_t virt = (uint64_t)address + hhdm_request.response->offset;
  switch (byte_width) {
  case 1:
    *out_value = *(volatile uint8_t *)virt;
    break;
  case 2:
    *out_value = *(volatile uint16_t *)virt;
    break;
  case 4:
    *out_value = *(volatile uint32_t *)virt;
    break;
  case 8:
    *out_value = *(volatile uint64_t *)virt;
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address,
                                           uacpi_u8 byte_width,
                                           uacpi_u64 in_value) {
  volatile uint64_t *virt =
      (volatile uint64_t *)((uint64_t)address + hhdm_request.response->offset);
  switch (byte_width) {
  case 1:
    *(volatile uint8_t *)virt = in_value;
    break;
  case 2:
    *(volatile uint16_t *)virt = in_value;
    break;
  case 4:
    *(volatile uint32_t *)virt = in_value;
    break;
  case 8:
    *(volatile uint64_t *)virt = in_value;
    break;
  default:
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address,
                                      uacpi_u8 byte_width,
                                      uacpi_u64 *out_value) {
  *out_value = arch_raw_io_in(address, byte_width);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address,
                                       uacpi_u8 byte_width,
                                       uacpi_u64 in_value) {
  arch_raw_io_write(address, in_value, byte_width);
  return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle dev) {
  kfree((void *)(uint64_t)dev, sizeof(uacpi_pci_address));
}
#warning When u want to port other archiectures, ADD ECAM for pci, dont know what that is? look at the osdev.wiki about it
uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset,
                                    uacpi_u8 *value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  *value = pciconfigreadbyte(address->bus, address->device, address->function,
                             offset);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset,
                                     uacpi_u16 *value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  *value = pciconfigreadword(address->bus, address->device, address->function,
                             offset);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset,
                                     uacpi_u32 *value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  *value =
      pciconfigread32(address->bus, address->device, address->function, offset);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset,
                                     uacpi_u8 value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  pciconfigwritebyte(address->bus, address->device, address->function, offset,
                     (uint8_t)value);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset,
                                      uacpi_u16 value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  pciconfigwriteword(address->bus, address->device, address->function, offset,
                     (uint8_t)value);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset,
                                      uacpi_u32 value) {
  uacpi_pci_address *address = (uacpi_pci_address *)device;
  pciconfigwrite32(address->bus, address->device, address->function, offset,
                   (uint8_t)value);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address,
                                          uacpi_handle *out_handle) {
  uacpi_pci_address *ptr = kmalloc(sizeof(uacpi_pci_address));
  *ptr = address;
  *out_handle = (uacpi_handle)(uint64_t)ptr;
  return UACPI_STATUS_OK;
}

struct io_range {
  uacpi_io_addr base;
  uacpi_size len;
};
uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
                                 uacpi_handle *out_handle) {
  struct io_range *rng = kmalloc(sizeof(struct io_range));
  rng->base = base;
  rng->len = len;
  *out_handle = (uacpi_handle)rng;
  return UACPI_STATUS_OK;
}
void uacpi_kernel_io_unmap(uacpi_handle handle) {
  kfree((void *)(struct io_range *)handle, sizeof(struct io_range));
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
  return (void *)((uint64_t)addr + hhdm_request.response->offset);
}
void uacpi_kernel_unmap(void *addr, uacpi_size len) {
  // nothing
}
uacpi_status uacpi_kernel_io_read8(uacpi_handle io_range, uacpi_size offset,
                                   uacpi_u8 *out_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  *out_value = (uint8_t)arch_raw_io_in(rng->base + offset, 1);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_write8(uacpi_handle io_range, uacpi_size offset,
                                    uacpi_u8 in_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  arch_raw_io_write(rng->base + offset, in_value, 1);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_write16(uacpi_handle io_range, uacpi_size offset,
                                     uacpi_u16 in_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  arch_raw_io_write(rng->base + offset, in_value, 2);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_write32(uacpi_handle io_range, uacpi_size offset,
                                     uacpi_u32 in_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  arch_raw_io_write(rng->base + offset, in_value, 4);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_read16(uacpi_handle io_range, uacpi_size offset,
                                    uacpi_u16 *out_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  *out_value = (uint16_t)arch_raw_io_in(rng->base + offset, 2);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_io_read32(uacpi_handle io_range, uacpi_size offset,
                                    uacpi_u32 *out_value) {
  struct io_range *rng = (struct io_range *)io_range;
  if (offset >= rng->len) {
    return UACPI_STATUS_INVALID_ARGUMENT;
  }
  *out_value = (uint32_t)arch_raw_io_in(rng->base + offset, 4);
  return UACPI_STATUS_OK;
}
void *uacpi_kernel_alloc(uacpi_size size) { return kmalloc(size); }
void *uacpi_kernel_calloc(uacpi_size count, uacpi_size size) {
  void *man = kmalloc(count * size);
  return man;
}
void uacpi_kernel_free(void *mem, uacpi_size size_hint) {
  kfree(mem, size_hint);
}
void uacpi_kernel_log(uacpi_log_level log, const uacpi_char *cha) {
  kprintf("uacpi: %s", (char *)cha);
}
uacpi_u64 uacpi_kernel_get_ticks(void) { return 46644; }

void uacpi_kernel_stall(uacpi_u8 usec) {
  int ret = GenericTimerStallPollus(usec);
  if (ret == -1) {
    panic("uacpi_kernel_stall cannot continue further without StallPollus "
          "(Perhaps its unimplmented?)");
  }
}
void uacpi_kernel_sleep(uacpi_u64 msec) {
  // now i could implmenet sleep but nah more impomortent fuck you
  GenericTimerStallPollms(msec);
}
uacpi_handle uacpi_kernel_create_mutex(void) {
  spinlock_t *mutex = kmalloc(sizeof(spinlock_t));
  return mutex;
}
void uacpi_kernel_free_mutex(uacpi_handle f) {
  kfree((void *)f, sizeof(spinlock_t));
}
void uacpi_kernel_free_spinlock(uacpi_handle f) {
  kfree((void *)f, sizeof(spinlock_t));
}
uacpi_handle uacpi_kernel_create_event(void) { return (void *)1; }

void uacpi_kernel_free_event(uacpi_handle) {}

uacpi_thread_id uacpi_kernel_get_thread_id(void) { return 0; }
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle thing, uacpi_u16 w) {
  spinlock_lock((spinlock_t *)thing);
  return UACPI_STATUS_OK;
}
void uacpi_kernel_release_mutex(uacpi_handle thing) {
  spinlock_unlock((spinlock_t *)thing);
}
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) { return true; }
void uacpi_kernel_signal_event(uacpi_handle) {}
void uacpi_kernel_reset_event(uacpi_handle) {}
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *) {
  return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle) {
  int ret = uacpi_arch_install_irq(irq, handler, ctx, out_irq_handle);
  if (ret == -1) {
    return UACPI_STATUS_NO_HANDLER;
  }

  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler,
                                                      uacpi_handle irq_handle) {
  return UACPI_STATUS_UNIMPLEMENTED;
}
uacpi_handle uacpi_kernel_create_spinlock(void) {
  spinlock_t *mutex = kmalloc(sizeof(spinlock_t));
  return mutex;
}
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle lock) {
  __asm__ volatile("cli");
  spinlock_lock((spinlock_t *)lock);
  return 1;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle lock, uacpi_cpu_flags) {
  spinlock_unlock((spinlock_t *)lock);
  __asm__ volatile("sti");
}
uacpi_status uacpi_kernel_schedule_work(uacpi_work_type t, uacpi_work_handler f,
                                        uacpi_handle ctx) {
  f(ctx);
  return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_wait_for_work_completion(void) {
  kprintf("uacpi called wait for work completion\r\n");
  return UACPI_STATUS_OK;
}
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
  return GenericTimerGetns();
}
