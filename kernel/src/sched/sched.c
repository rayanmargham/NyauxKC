#include "sched.h"

#include <mem/kmem.h>
#include <stdint.h>

#include "arch/arch.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/instructions/instructions.h"
#include "mem/vmm.h"
#include "sched.h"
#include "sched/reaper.h"
#include "smp/smp.h"
#include "utils/basic.h"
// ARCH STUFF
#include "arch/x86_64/cpu/structures.h"

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
	him->cnt = 0;
	return him;
}
struct thread_t* create_thread()
{
	struct thread_t* him = (struct thread_t*)kmalloc(sizeof(struct thread_t));
	him->count = 2;
	// him->next = NULL;
	return him;
}
void push_into_list(struct thread_t** list, struct thread_t* whatuwannapush)
{
	if (*list == NULL)
	{
		(*list) = whatuwannapush;
		(*list)->next = *list;
		(*list)->back = *list;
		return;
	}
	struct thread_t* temp = (*list);
	struct thread_t* old = (*list);
	while (temp->next != old)
	{
		temp = temp->next;
	}
	temp->next = whatuwannapush;
	whatuwannapush->back = temp;
	whatuwannapush->next = old;
	old->back = whatuwannapush;
}
struct thread_t* pop_from_list(struct thread_t** list)
{
	struct thread_t* old = *list;
	*list = (*list)->next;
	(*list)->back = old->back;
	(*list)->back->next = *list;
	return old;
}
#if defined(__x86_64)

#endif
void exit_thread()
{
	__asm__ volatile("cli");
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	cpu->cur_thread->state = ZOMBIE;
	assert(cpu->cur_thread->proc != NULL);
	push_into_list(&cpu->to_be_reapered, cpu->cur_thread);
	// assert(cpu->to_be_reapered != NULL);
	assert(cpu->to_be_reapered->proc != NULL);
	refcount_dec(&cpu->to_be_reapered->proc->cnt);
	refcount_dec(&cpu->to_be_reapered->count);
	refcount_dec(&cpu->to_be_reapered->count);
	// hcf();
	// no need to assert
	// reaper thread is always running
	sched_yield();
}

void ThreadBlock(struct thread_t* whichqueue)
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	cpu->cur_thread->state = BLOCKED;
	push_into_list(&whichqueue, cpu->cur_thread);
}
void ThreadReady(struct thread_t* thread)
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	thread->state = READYING;

	push_into_list(&cpu->run_queue, thread);
}
void create_kthread(uint64_t entry, struct process_t* proc, uint64_t tid)
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	struct thread_t* newthread = create_thread();
	newthread->proc = proc;
	newthread->tid = tid;
	refcount_inc(&proc->cnt);
	uint64_t kstack = (uint64_t)(kmalloc(KSTACKSIZE) + KSTACKSIZE);
	struct StackFrame hh = arch_create_frame(false, entry, kstack - 8);
	newthread->kernel_stack_base = kstack;
	newthread->kernel_stack_ptr = kstack;
	newthread->arch_data.frame = hh;
	refcount_inc(&newthread->count);
	refcount_inc(&newthread->count);
	ThreadReady(newthread);
}
void create_kentry()
{
#if defined(__x86_64__)
	struct process_t* kernelprocess = create_process(&ker_map);
	// kprintf("ran fine\r\n");
	create_kthread((uint64_t)kentry, kernelprocess, 1);
	create_kthread((uint64_t)reaper, kernelprocess, 0);
	create_kthread((uint64_t)klocktest, kernelprocess, 2);
	create_kthread((uint64_t)klocktest2, kernelprocess, 3);

	// e->back = e;
	// e->next = e;
	// cpu->run_queue = e;
	// assert(cpu->run_queue->next != NULL);
	// refcount_inc(&e->count);

#endif
}

// returns the old thread
struct thread_t* switch_queue(struct per_cpu_data* cpu)
{
	if (cpu->cur_thread)
	{
		struct thread_t* old = cpu->cur_thread;
		if (cpu->cur_thread->state == ZOMBIE || cpu->cur_thread->state == BLOCKED)
		{
			cpu->cur_thread = pop_from_list(&cpu->run_queue);
			cpu->cur_thread->state = RUNNING;
			return old;
		}

		cpu->cur_thread->state = READY;
		push_into_list(&cpu->run_queue, cpu->cur_thread);
		cpu->cur_thread = pop_from_list(&cpu->run_queue);
		if (cpu->cur_thread->state == READYING)
		{
			cpu->cur_thread->state = READY;
			push_into_list(&cpu->run_queue, cpu->cur_thread);
			cpu->cur_thread = pop_from_list(&cpu->run_queue);
			cpu->cur_thread->state = RUNNING;
			return old;
		}
		cpu->cur_thread->state = RUNNING;
		return old;
	}
	else
	{
		cpu->cur_thread = pop_from_list(&cpu->run_queue);
		cpu->cur_thread->state = RUNNING;
		return NULL;
	}
}
#ifdef __x86_64__
void load_ctx(struct StackFrame* context);
void save_ctx(struct StackFrame* old_context, struct StackFrame* new_context);
#endif
void schedd(struct StackFrame* frame)
{
	struct per_cpu_data* cpu = arch_get_per_cpu_data();
	if (cpu->cur_thread == NULL && cpu->run_queue == NULL)
	{
		// kprintf("schedd(): no threads to run\r\n");
		return;
	}
	struct thread_t* old = switch_queue(cpu);
#if defined(__x86_64__)
	if (old == NULL)
	{
		// save_ctx(NULL, &cpu->cur_thread->arch_data.frame);
	}
	else
	{
		// kprintf("rip of new: %p\r\nrip of old: %p\n", cpu->cur_thread->arch_data.frame.rip,
		// old->arch_data.frame.rip);
		// sprintf("old rip is %p\r\n", old->arch_data.frame.rip);
		save_ctx(frame, &old->arch_data.frame);
		// sprintf("new rip is %p\r\n", old->arch_data.frame.rip);
	}
	arch_switch_pagemap(cpu->cur_thread->proc->cur_map);
	send_eoi();
	load_ctx(&cpu->cur_thread->arch_data.frame);

#endif
	// arch_load_ctx(frame, cpu->run_queue);
	//  for reading operations such as switching the pagemap
	//  it is fine not to lock, otherwise we would have deadlocks and major slowdowns
	//  for writing anything however, the process MUST be locked
	// we dont ever return
}
