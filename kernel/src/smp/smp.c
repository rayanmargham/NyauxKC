#include "smp.h"

#include "arch/arch.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/instructions/instructions.h"
#include "sched/sched.h"
#include "timers/hpet.h"

void bootstrap(struct limine_mp_info* info)
{
	arch_late_init();

	hcf();
}
uint32_t bsp_id = 0;
void init_smp()
{
	for (uint64_t i = 0; i < smp_request.response->cpu_count; i++)
	{
		struct limine_mp_info* cpu = smp_request.response->cpus[i];
		cpu->goto_address = bootstrap;
	}
	bsp_id = smp_request.response->bsp_lapic_id;
	init_lapic();
}
