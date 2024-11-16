#include "smp.h"
#include "term/term.h"

void bootstrap(struct limine_smp_info *info) {
  kprintf("bootstrap(): Hello from CPU %d\n", info->lapic_id);
}
void init_smp() {
  for (uint64_t i = 0; i < smp_request.response->cpu_count; i++) {
    struct limine_smp_info *cpu = smp_request.response->cpus[i];
    cpu->goto_address = bootstrap;
    kprintf("init_smp(): Found CPU %d\n", cpu->lapic_id);
  }
}
