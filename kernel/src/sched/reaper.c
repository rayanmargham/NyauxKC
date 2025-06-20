#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"
void remove_from_parent_process(struct process_t *processtoremove) {
  if (processtoremove) {
    assert(processtoremove->parent != processtoremove);

          struct process_t *pid_1 = processtoremove;
          // the kernel process
          if (pid_1->pid == 0) {
            panic("attempted to kill init process\r\n");
          }
          while (pid_1->parent) {
            pid_1 = pid_1->parent;
          }
          assert(pid_1->pid == 0);
          // pid 1 now has to pay child support
          // as well as taking custody :^)
          pid_1->children = processtoremove->children;
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
      // kfree((uint64_t *)(reaper->kernel_stack_base - KSTACKSIZE), KSTACKSIZE);
      struct process_t *proc = reaper->proc;
      //spinlock_lock(&proc->lock);
      remove_from_parent_process(proc);
      if (!(proc->cur_map == &ker_map) &&
          proc->cur_map != cpu->cur_thread->proc->cur_map) {

        free_pagemap(proc->cur_map);
      }
      hashmap_free(proc->fds); // this was a memory leak i forgot existed :sob:
      // process is getting deleted anyway so yea no need to unlock, waste of puter cycle
      kfree(proc, sizeof(struct process_t));

      cpu->to_be_reapered = reaper->next;

      kfree(reaper, sizeof(struct thread_t));
      sprintf("reaper(): thread killed\r\n");

    }
  }
}
