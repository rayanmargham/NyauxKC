#include "sched.h"
#include "arch/arch.h"
#include "arch/x86_64/instructions/instructions.h"
#include "mem/vmm.h"
#include "utils/basic.h"
#include <mem/kmem.h>
#include <stdint.h>
// ARCH STUFF
#include "arch/x86_64/cpu/structures.h"



void arch_save_ctx(void *frame, struct thread_t *threadtosavectx) {
    #ifdef __x86_64__ 
    threadtosavectx->arch_data.frame = *(struct StackFrame*)frame;
    #endif
}
void arch_load_ctx(void *frame, struct thread_t *threadtoloadctxfrom) {
    #ifdef __x86_64__ 
    *(struct StackFrame*)frame = threadtoloadctxfrom->arch_data.frame;
    #endif
}
struct per_cpu_data *arch_get_per_cpu_data() {
  #if defined (__x86_64__)
  struct per_cpu_data *hi = (struct per_cpu_data *)rdmsr(0xC0000101);
  assert(hi != NULL);
  return hi;
  #endif
}
void arch_create_per_cpu_data() {
    #if defined(__x86_64__)
    struct per_cpu_data *hey = (struct per_cpu_data *)kmalloc(sizeof(struct per_cpu_data));
    hey->run_queue = NULL;
    hey->start_of_queue = NULL;
    hey->arch_data.lapic_id = get_lapic_id();
    hey->arch_data.kernel_stack_ptr = 0;
    hey->arch_data.syscall_stack_ptr_tmp = 0;
    wrmsr(0xC0000101, (uint64_t)hey);
    #endif
}
struct process_t *create_process(pagemap *map) {
    struct process_t *him = (struct process_t *)kmalloc(sizeof(struct process_t));
    him->cur_map = map;
    him->lock = SPINLOCK_INITIALIZER;
    return him;
}
struct thread_t *create_thread() {
    struct thread_t *him = (struct thread_t*)kmalloc(sizeof(struct thread_t));
    //him->next = NULL;
    return him;
}
void create_kentry() {
    #if defined(__x86_64__)
    struct process_t *h = create_process(&ker_map);
    struct thread_t *e = create_thread();
    e->proc = h;
    uint64_t kstack = (uint64_t)(kmalloc(262144) + 262144); // top of stack
    struct StackFrame hh = arch_create_frame(false, (uint64_t)kentry, kstack);
    e->arch_data.frame = hh;
    struct per_cpu_data *cpu = arch_get_per_cpu_data();
    cpu->start_of_queue = e;
    #endif
}
void schedd(void *frame) {
    struct per_cpu_data *cpu = arch_get_per_cpu_data();
    if (cpu->run_queue == NULL && cpu->start_of_queue == NULL) {
        return;
    }
    if (cpu->run_queue != NULL) {
        arch_save_ctx(frame, cpu->run_queue);
    } else {
        cpu->run_queue = cpu->start_of_queue;
    }
    
    if (cpu->run_queue->next == NULL) {
        cpu->run_queue = cpu->start_of_queue;
    } else {
        cpu->run_queue = cpu->run_queue->next;
    }
    arch_load_ctx(frame, cpu->run_queue);
    // for reading operations such as switching the pagemap
    // it is fine not to lock, otherwise we would have deadlocks and major slowdowns
    // for writing anything however, the process MUST be locked
    arch_switch_pagemap(cpu->run_queue->proc->cur_map);


}
