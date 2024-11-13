#include "hpet.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "uacpi/acpi.h"
#include "uacpi/helpers.h"
#include "uacpi/status.h"
#include "uacpi/tables.h"
#include <stdint.h>

uint64_t ctr_clock_period = 0; // in nano seconds
uint64_t *hpetvirtaddr = NULL;
void stall_with_hpetclk(uint64_t ms) {
  uint64_t pol_start = *(uint64_t *)((uint64_t)hpetvirtaddr + 0xf0);
  uint64_t *pol_cur = (uint64_t *)((uint64_t)hpetvirtaddr + 0xf0);
  while ((*pol_cur - pol_start) * ctr_clock_period < ms * 1000000) {
  }
}
void init_hpet() {
  if (hpetvirtaddr != NULL) {
    return;
  }
  uacpi_table hpet_table;
  uacpi_status st = uacpi_table_find_by_signature("HPET", &hpet_table);
  if (st != UACPI_STATUS_OK) {
    panic("The Nyaux Kernel does not support Devices without a HPET\nThe "
          "Reason for this is becausei hate tsc with a burning desire. pr in a "
          "different calibration timer if u want one so bad lol");
  }
  kprintf("init_hpet(): Timer Table Found\n");
  struct acpi_hpet *hpet = (struct acpi_hpet *)hpet_table.virt_addr;
  kprintf("init_hpet(): HPET has Physical Address 0x%lx\n",
          hpet->address.address);
  uint32_t *cap =
      (uint32_t *)(hpet->address.address + hhdm_request.response->offset);
  if (!(*cap & (1 << 13))) {
    kprintf("init_hpet(): Bit 13 is off\ninit_hpet(): Panicking.\n");
    panic("Not a 32 bit HPET buddy.");
  }
  uint64_t *capr =
      (uint64_t *)(hpet->address.address + hhdm_request.response->offset);
  ctr_clock_period = ((uint64_t)(*capr >> 32)) / 1000000;
  kprintf("init_hpet(): Counter Clock Period %lu\n", ctr_clock_period);
  capr = (uint64_t *)(hpet->address.address + hhdm_request.response->offset +
                      0x10);
  *capr |= 1; // enable counter

  kprintf("init_hpet(): Main Counter Enabled!\n");
  hpetvirtaddr =
      (uint64_t *)(hpet->address.address + hhdm_request.response->offset);
}
