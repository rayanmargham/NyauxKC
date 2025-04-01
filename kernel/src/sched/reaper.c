#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"
void remove_from_process_list(struct per_cpu_data *cpu,
                              struct process_t *processtoremove) {
  if (cpu) {
    struct process_t *process_list = cpu->process_list;
    struct process_t *previous = NULL;
    while (process_list) {
      if (process_list == processtoremove) {
        if (!previous) {
          cpu->process_list = processtoremove->next;
        } else {
          previous->next = processtoremove->next;
        }
      }
      previous = process_list;
      process_list = process_list->next;
    }
  }
}
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

    if (reaper->proc->cnt <= 0 &&
        (reaper->proc->state != ZOMBIE && reaper->proc->state == BLOCKED)) {
      kfree((uint64_t *)(reaper->kernel_stack_base - KSTACKSIZE), KSTACKSIZE);
      struct process_t *proc = reaper->proc;
      remove_from_process_list(cpu, proc);
      if (!(proc->cur_map == &ker_map)) {
        kprintf("proc_addr: %#llx, tid: %u, proc_as: %#llx, kern_as: %#llx\n",
                proc, reaper->tid, proc->cur_map, &ker_map);
        kprintf("reaper(): Freeing a PageMap is ENOSYS\r\r\n");
      }
      kfree(proc, sizeof(struct process_t));

      cpu->to_be_reapered = reaper->next;

      kfree(reaper, sizeof(struct thread_t));
      kprintf("reaper(): thread killed\r\n");
    }
  }
}
