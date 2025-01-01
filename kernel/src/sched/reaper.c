#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"

void reaper()
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	kprintf("reaper(): I am online and ready to KILL any threads that are in the zombie queue\n");
	if (cpu->to_be_reapered)
	{
		struct thread_t* reaper = cpu->to_be_reapered;
		while (reaper != NULL)
		{
			// kprintf("gggg : %p\r\n", (uint64_t*)reaper->kernel_stack_base);
			// kfree((uint64_t*)reaper->kernel_stack_base, KSTACKSIZE);
			if (reaper->count == 0)
			{
				kfree((uint64_t*)reaper->kernel_stack_base, KSTACKSIZE);
				if (reaper->proc->cnt == 0)
				{
					struct process_t* proc = reaper->proc;
					if (!(proc->cur_map == &ker_map))
					{
						kprintf("reaper(): Freeing a PageMap is ENOSYS\r\n");
					}
					kfree(proc, sizeof(struct process_t));
				}
				struct thread_t* stay = reaper->next;
				kfree(reaper, sizeof(struct thread_t));
				reaper = stay;
			}
		}
	}
	hcf();
}
