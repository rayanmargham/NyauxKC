#include "smp.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "mem/vmm.h"
#include "timers/hpet.h"
#include "timers/lapic.h"
#include <stdint.h>

void bootstrap(struct limine_smp_info *info) {
  kprintf("bootstrap(): Hello from CPU %d\n", get_lapic_id());
  init_gdt();
  init_idt();
  per_cpu_vmm_init();
  init_lapic();
  hcf();
}
void init_smp() {
  for (uint64_t i = 0; i < smp_request.response->cpu_count; i++) {
    struct limine_smp_info *cpu = smp_request.response->cpus[i];
    cpu->goto_address = bootstrap;
    kprintf("init_smp(): Found CPU %d\n", cpu->lapic_id);
  }
  stall_with_hpetclk(100);
  init_lapic();
}
