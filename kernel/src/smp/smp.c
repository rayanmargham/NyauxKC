#include "smp.h"
#include "arch/arch.h"

void bootstrap(struct limine_smp_info *info) {
  arch_late_init();
  hcf();
}
void init_smp() {
  for (uint64_t i = 0; i < smp_request.response->cpu_count; i++) {
    struct limine_smp_info *cpu = smp_request.response->cpus[i];
    cpu->goto_address = bootstrap;
  }
}
