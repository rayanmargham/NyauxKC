#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"

void reaper()
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	while (1) {
	if (cpu->to_be_reapered) {
		struct thread_t* reaper = cpu->to_be_reapered;
		while (reaper != NULL)
		{
			// kprintf("gggg : %p\r\r\n", (uint64_t*)reaper->kernel_stack_base);
			// kfree((uint64_t*)reaper->kernel_stack_base, KSTACKSIZE);
			if (reaper->count == 0)
			{
				assert(cpu->to_be_reapered->kernel_stack_base != cpu->cur_thread->kernel_stack_base);

				kfree((uint64_t*)(reaper->kernel_stack_base - KSTACKSIZE), KSTACKSIZE);
				if (reaper->proc->cnt == 0)
				{
					struct process_t* proc = reaper->proc;
					if (!(proc->cur_map == &ker_map))
					{
						kprintf("reaper(): Freeing a PageMap is ENOSYS\r\r\n");
					}
					kfree(proc, sizeof(struct process_t));
				}
				struct thread_t* stay = reaper->next;
				if (stay == reaper)
				{
					cpu->to_be_reapered = NULL;
					kfree(reaper, sizeof(struct thread_t));
					reaper = NULL;
				kprintf("reaper(): thread killed\r\n");
				break;
				}
				kfree(reaper, sizeof(struct thread_t));
				kprintf("reaper(): thread killed\r\n");
				
				reaper = stay;
			}
		}
	} }
	
}
