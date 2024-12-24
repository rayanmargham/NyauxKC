#pragma once
#include <arch/arch.h>
#include <stdint.h>

#include "arch/x86_64/instructions/instructions.h"
#include "utils/basic.h"
struct process_t
{
	pagemap* cur_map;
	spinlock_t lock;	// lock for accessing this
};
struct thread_t
{
	struct process_t* proc;	   // there should be a lock on this
	struct arch_thread_t arch_data;
	uint64_t kernel_stack_ptr;
	uint64_t kernel_stack_base;
	struct thread_t* next;
};
struct per_cpu_data
{
	struct arch_per_cpu_data arch_data;
	struct thread_t* run_queue;			// real run queue
	struct thread_t* start_of_queue;	// contains just the start of the queue
};

void schedd(void* frame);
void arch_create_per_cpu_data();
extern void kentry();
void create_kentry();
struct per_cpu_data* arch_get_per_cpu_data();
