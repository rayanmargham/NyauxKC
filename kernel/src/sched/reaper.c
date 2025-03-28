#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"

void reaper() {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  for (;;) {
#if defined(__x86_64__)
    __asm__ volatile("pause");
#endif

    struct thread_t *reaper = cpu->to_be_reapered;
    if (!reaper)
      continue;
    if (reaper->count != 0)
      continue;

    assert(cpu->to_be_reapered->kernel_stack_base !=
           cpu->cur_thread->kernel_stack_base);

    kfree((uint64_t *)(reaper->kernel_stack_base - KSTACKSIZE), KSTACKSIZE);
    if (reaper->proc->cnt == 0 &&
        (reaper->proc->state != ZOMBIE && reaper->proc->state != BLOCKED)) {
      struct process_t *proc = reaper->proc;
      if (!(proc->cur_map == &ker_map)) {
        kprintf("proc_addr: %#llx, tid: %u, proc_as: %#llx, kern_as: %#llx\n",
                proc, reaper->tid, proc->cur_map, &ker_map);
        kprintf("reaper(): Freeing a PageMap is ENOSYS\r\r\n");
      }
      kfree(proc, sizeof(struct process_t));
    }

    cpu->to_be_reapered = reaper->next;

    kfree(reaper, sizeof(struct thread_t));
    kprintf("reaper(): thread killed\r\n");
  }
}
