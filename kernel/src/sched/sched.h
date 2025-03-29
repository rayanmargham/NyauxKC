#pragma once
#include <arch/arch.h>
#include <stdint.h>

#include "arch/x86_64/cpu/structures.h"
#include "arch/x86_64/instructions/instructions.h"
#include "utils/basic.h"
#include "utils/hashmap.h"
#define KSTACKSIZE 16384
enum TASKSTATE {
  READY = 0,
  RUNNING = 1,
  ZOMBIE = 2,
  BLOCKED = 3,
  KILLED = 4,
  READYING = 5,
};
struct process_t {
  uint64_t pid;
  enum TASKSTATE state;
  pagemap *cur_map;
  spinlock_t lock; // lock for accessing this
  refcount_t cnt;
  struct hashmap *fds;
  int fdalloc[255];
  uint64_t exit_code;
  struct vnode *root;
  struct vnode *cwd; // current working directory
  char *cwdpath;
  struct thread_t
      *queuewaitingforexit; // threads waiting for the process to exit
  struct process_t *next;   // FUCK YOU!!! :)
};
struct thread_t {
  struct process_t *proc; // there should be a lock on this
  struct arch_thread_t arch_data;
  uint64_t kernel_stack_ptr;
  uint64_t kernel_stack_base;
  struct thread_t *next, *back;
  uint64_t tid;
  enum TASKSTATE state;
  refcount_t count;
  void *fpu_state;
};
struct per_cpu_data {
  struct arch_per_cpu_data arch_data;
  struct thread_t *run_queue;  // real run queue
  struct thread_t *cur_thread; // trl
  struct thread_t
      *to_be_reapered; // any thread with refcount zero will be thrown into here
  // a reaper thread will kill the dead
  struct process_t *process_list
};
struct fpu_state {
  uint16_t fcw;
  uint32_t rev0;
  uint16_t rev;
  uint64_t rev2;
  uint64_t rev3;
  uint32_t mxcsr;
} __attribute__((packed));
void schedd(struct StackFrame *frame);
void arch_create_per_cpu_data();
extern void kentry();
extern void klocktest2();
extern void klocktest();
void create_kentry();
struct per_cpu_data *arch_get_per_cpu_data();
void exit_thread();
void ThreadBlock(struct thread_t *whichqueue);
struct thread_t *pop_from_list(struct thread_t **list);
void push_into_list(struct thread_t **list, struct thread_t *whatuwannapush);
void ThreadReady(struct thread_t *thread);
#ifdef __x86_64__
extern void sched_yield();
void create_kthread(uint64_t entry, struct process_t *proc, uint64_t tid);
#endif
int scheduler_fork();
void exit_process(uint64_t exit_code);
struct process_t *get_process_start();
void get_process_finish(struct process_t *proc);
void arch_create_bsp_per_cpu_data();
void clear_and_prepare_thread(struct thread_t *t);