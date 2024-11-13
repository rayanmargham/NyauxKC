#include <mem/kmem.h>
#include <stdint.h>
#define UACPI_SIZED_FREES

#include <uacpi/kernel_api.h>
#include <utils/basic.h>
extern volatile struct limine_rsdp_request rsdp_request;
typedef struct {
  uacpi_interrupt_handler fn;
  uacpi_handle ctx;
} uacpi_irq_wrap_info;
