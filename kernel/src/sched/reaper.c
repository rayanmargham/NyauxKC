#include "reaper.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "sched.h"
#include "utils/basic.h"
void remove_from_parent_process(struct process_t *processtoremove) {
  if (processtoremove) {
    assert(processtoremove->parent != processtoremove);
    struct process_t *parent = processtoremove->parent->children;
    struct process_t *previous = NULL;
    while (parent) {
      if (parent == processtoremove) {
        if (!previous) {

          parent->parent->children = processtoremove->children;
        } else {
          previous->parent = processtoremove->parent;
          previous->parent->children = processtoremove->children;
        }
      }
      previous = parent;
      parent = parent->children;
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
      remove_from_parent_process(proc);
      if (!(proc->cur_map == &ker_map) &&
          proc->cur_map != cpu->cur_thread->proc->cur_map) {

        free_pagemap(proc->cur_map);
      }
      hashmap_free(proc->fds); // this was a memory leak i forgot existed :sob:
      kfree(proc, sizeof(struct process_t));

      cpu->to_be_reapered = reaper->next;

      kfree(reaper, sizeof(struct thread_t));
      sprintf(__func__ "(): thread killed\r\n");
    }
  }
}
