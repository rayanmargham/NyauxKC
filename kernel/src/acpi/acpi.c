#include "acpi.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "uacpi/types.h"
#include "uacpi/uacpi.h"
void init_acpi() {
  uint64_t rsdp =
      (uint64_t)rsdp_request.response->address - hhdm_request.response->offset;
  kprintf("initing uacpi\n");
  uacpi_status st = uacpi_initialize(0);
  kprintf("now loading namespace\n");
  if (st == UACPI_STATUS_OK) {
    st = uacpi_namespace_load();
    st = uacpi_namespace_initialize();
    kprintf("uacpi finsihed\n");

  } else {
    panic("Failed");
  }
}