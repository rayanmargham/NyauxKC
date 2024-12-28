#include "sched.h"

#include <mem/kmem.h>
#include <stdint.h>

#include "arch/arch.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/instructions/instructions.h"
#include "mem/vmm.h"
#include "smp/smp.h"
#include "utils/basic.h"
// ARCH STUFF
#include "arch/x86_64/cpu/structures.h"

#ifdef __x86_64__
extern void do_savekstackandloadkstack(struct thread_t* old, struct thread_t* new);
#endif
void arch_save_ctx(void* frame, struct thread_t* threadtosavectx)
{
#ifdef __x86_64__
	// threadtosavectx->arch_data.frame = *(struct StackFrame*)frame;
	//__asm__ volatile("mov %%rsp, %0" : : "r"(threadtosavectx->kernel_stack_ptr));
#endif
}
void arch_load_ctx(void* frame, struct thread_t* threadtoloadctxfrom)
{
#ifdef __x86_64__
	// *(struct StackFrame*)frame = threadtoloadctxfrom->arch_data.frame;

	//__asm__ volatile("mov %0, %%rsp" : : "r"(threadtoloadctxfrom->kernel_stack_ptr));
#endif
}
#if defined(__x86_64__)
struct per_cpu_data bsp = {.run_queue = NULL};
#endif
struct per_cpu_data* arch_get_per_cpu_data()
{
#if defined(__x86_64__)
	if (get_lapic_id() == bsp_id)
	{
		assert(&bsp != NULL);
		return &bsp;
	}
	struct per_cpu_data* hi = (struct per_cpu_data*)rdmsr(0xC0000101);
	return hi;
#endif
}
void arch_create_per_cpu_data()
{
#if defined(__x86_64__)
	struct per_cpu_data* hey = (struct per_cpu_data*)kmalloc(sizeof(struct per_cpu_data));
	hey->run_queue = NULL;
	hey->start_of_queue = NULL;
	hey->arch_data.lapic_id = get_lapic_id();
	hey->arch_data.kernel_stack_ptr = 0;
	hey->arch_data.syscall_stack_ptr_tmp = 0;
	wrmsr(0xC0000101, (uint64_t)hey);
#endif
}
struct process_t* create_process(pagemap* map)
{
	struct process_t* him = (struct process_t*)kmalloc(sizeof(struct process_t));
	him->cur_map = map;
	him->lock = SPINLOCK_INITIALIZER;
	return him;
}
struct thread_t* create_thread()
{
	struct thread_t* him = (struct thread_t*)kmalloc(sizeof(struct thread_t));
	// him->next = NULL;
	return him;
}
#if defined(__x86_64)
extern void return_from_kernel_in_new_thread();
void load_ctx_into_kstack(struct thread_t* t, struct StackFrame usrctx)
{
	t->kernel_stack_ptr -= sizeof(struct StackFrame);
	*(struct StackFrame*)t->kernel_stack_ptr = usrctx;
	t->kernel_stack_ptr -= sizeof(uint64_t*);
	*(uint64_t*)t->kernel_stack_ptr = (uint64_t)return_from_kernel_in_new_thread;
	t->kernel_stack_ptr -= sizeof(uint64_t) * 6;
}
#endif

void create_kentry()
{
#if defined(__x86_64__)
	struct process_t* h = create_process(&ker_map);
	struct thread_t* e = create_thread();
	e->proc = h;
	// 16kib kstack lol
	uint64_t kstack = (uint64_t)(kmalloc(16384) + 16384);	 // top of stack
	// profiler will crash because it expects a return address
	struct StackFrame hh = arch_create_frame(false, (uint64_t)kentry, kstack - 8);
	e->kernel_stack_base = kstack;
	e->kernel_stack_ptr = kstack;
	e->arch_data.frame = hh;
	load_ctx_into_kstack(e, hh);
	kprintf("ran fine\n");
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	cpu->start_of_queue = e;
#endif
}
void schedd(void* frame)
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	if (cpu->run_queue == NULL && cpu->start_of_queue == NULL)
	{
		return;
	}
	if (cpu->run_queue != NULL)
	{
		// arch_save_ctx(frame, cpu->run_queue);

		if (cpu->run_queue->next == NULL)
		{
			struct thread_t* tmp = cpu->run_queue;
			cpu->run_queue = cpu->start_of_queue;
			cpu->arch_data.kernel_stack_ptr = cpu->run_queue->kernel_stack_base;
			arch_switch_pagemap(cpu->run_queue->proc->cur_map);
// save and load
#ifdef __x86_64__
			send_eoi();
			do_savekstackandloadkstack(tmp, cpu->run_queue);
#endif
		}
		else
		{
			struct thread_t* tmp = cpu->run_queue;
			cpu->run_queue = cpu->run_queue->next;
			cpu->arch_data.kernel_stack_ptr = cpu->run_queue->kernel_stack_base;
			arch_switch_pagemap(cpu->run_queue->proc->cur_map);
// save and load
#ifdef __x86_64__
			send_eoi();
			do_savekstackandloadkstack(tmp, cpu->run_queue);
#endif
		}
	}
	else
	{
		struct thread_t* tmp = cpu->start_of_queue;
		cpu->run_queue = cpu->start_of_queue;
		cpu->arch_data.kernel_stack_ptr = cpu->run_queue->kernel_stack_base;
		arch_switch_pagemap(cpu->run_queue->proc->cur_map);
// save and load
#ifdef __x86_64__
		send_eoi();
		do_savekstackandloadkstack(NULL, cpu->run_queue);
#endif
	}
	// arch_load_ctx(frame, cpu->run_queue);
	//  for reading operations such as switching the pagemap
	//  it is fine not to lock, otherwise we would have deadlocks and major slowdowns
	//  for writing anything however, the process MUST be locked
	// we dont ever return
}
