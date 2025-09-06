#include "sched.h"

#include <elf/elf.h>
#include <mem/kmem.h>
#include <stdint.h>

#include "arch/arch.h"
#include "arch/x86_64/cpu/lapic.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/instructions/instructions.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "mem/vmm.h"
#include "sched.h"
#include "sched/reaper.h"
#include "smp/smp.h"
#include "utils/basic.h"
// ARCH STUFF
#include <arch/x86_64/fpu/xsave.h>

#include "arch/x86_64/cpu/structures.h"

#if defined(__x86_64__)
struct per_cpu_data bsp = {.run_queue = NULL};
#endif
struct per_cpu_data *arch_get_per_cpu_data() {
#if defined(__x86_64__)
  struct per_cpu_data *hi = (struct per_cpu_data *)rdmsr(x86_KERNEL_GS_BASE);
  return hi;
#endif
}

void arch_create_bsp_per_cpu_data() {
  wrmsr(x86_KERNEL_GS_BASE, (uint64_t)&bsp);
}
void arch_create_per_cpu_data() {
#if defined(__x86_64__)
  struct per_cpu_data *hey =
      (struct per_cpu_data *)kmalloc(sizeof(struct per_cpu_data));
  hey->run_queue = NULL;
  hey->arch_data.lapic_id = get_lapic_id();
  hey->arch_data.kernel_stack_ptr = 0;
  hey->arch_data.syscall_stack_ptr_tmp = 0;
  wrmsr(x86_KERNEL_GS_BASE, (uint64_t)hey);
#endif
}

uint64_t pidalloc = 0;
spinlock_t pidlock = SPINLOCK_INITIALIZER;
uint64_t pidallocate() { spinlock_lock(&pidlock); uint64_t decided = pidalloc++; spinlock_unlock(&pidlock); return decided; }
//uint64_t piddealloc() { spinlock_lock(&pidlock); uint64_t decided = pidalloc--; spinlock_unlock(&pidlock); return decided; }

// TODO: a proper pid allocation system of some kind
uint64_t piddealloc() { return pidalloc; }

struct process_t *create_process(pagemap *map) {
  struct process_t *him = (struct process_t *)kmalloc(sizeof(struct process_t));
  him->cur_map = map;
  him->pid = pidallocate();
  kprintf("allocated to pid %lu\r\n", him->pid);
  him->lock = SPINLOCK_INITIALIZER;
  him->cnt = 0;
  him->fds = hashmap_new(sizeof(struct hfd), 0, 0, 0, fd_hash,
                         fd_compare, NULL, NULL);
  int stdin = ialloc_fd_struct(him);
  int stdout = ialloc_fd_struct(him);
  int stderr = ialloc_fd_struct(him);

  him->children = NULL;
  him->children_next = NULL;
  if (vfs_list) {
    him->root = vfs_list->cur_vnode;
    him->cwd = him->root;
    him->cwdpath = "/";
  }

  return him;
}

struct thread_t *create_thread() {
  struct thread_t *him = (struct thread_t *)kmalloc(sizeof(struct thread_t));
  him->count = 0;
  // him->next = NULL;
  return him;
}
void push_into_list(struct thread_t **list, struct thread_t *whatuwannapush) {
  if (*list == NULL) {
    (*list) = whatuwannapush;
    (*list)->next = *list;
    (*list)->back = *list;
    return;
  }
  // thanks mr gpt for somehow finding a bug here???
  // might be due to the circular list not being mantained properly ig?
  // gdb said it was fine but the while loop didnt terminate ????
  // idk but thanks mr gpt i didnt wanna spend hours debugging that anyway
  // more syscalls time :)
  struct thread_t *tail = (*list)->back;
  tail->next = whatuwannapush;
  whatuwannapush->back = tail;
  whatuwannapush->next = *list;
  (*list)->back = whatuwannapush;
}
struct thread_t *pop_from_list(struct thread_t **list) {
  struct thread_t *old = *list;
  *list = (*list)->next;
  (*list)->back = old->back;
  (*list)->back->next = *list;
  return old;
}
void exit_thread() {
  arch_disable_interrupts();
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  cpu->cur_thread->state = ZOMBIE;
  assert(cpu->cur_thread->proc != NULL);

  // assert(cpu->to_be_reapered != NULL);
  refcount_dec(&cpu->cur_thread->count);
  // refcount_dec(&cpu->cur_thread->count);
  
  cpu->cur_thread->next = cpu->to_be_reapered;
  cpu->to_be_reapered = cpu->cur_thread;

  // hcf();
  // no need to assert
  // reaper thread is always running

  sched_yield();
}
void exit_process(uint64_t exit_code) {
  // Turn the process into a zombie process
  // send all threads to the reaper to be exterminated
  // set process state to zombie
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  struct process_t *cur_proc = cpu->cur_thread->proc;
  struct thread_t *ptr = cpu->run_queue;
  struct thread_t *previous = ptr->back;
  while (ptr != NULL && ptr != previous) {
    if (ptr->proc == cur_proc) {
      struct thread_t *got = ptr->next;
      struct thread_t *back = ptr->back;
      back->next = got;
      ptr->back = NULL;
      ptr->next = NULL;
      kprintf_log(TRACE, "thread %lu\r\n", ptr->tid);
      // terminate thread
      refcount_dec(&ptr->count);
      refcount_dec(&ptr->count);
      refcount_dec(&cur_proc->cnt);
      ptr->next = cpu->to_be_reapered;
      cpu->to_be_reapered = ptr;
      ptr = got;
      continue;
    }
    ptr = ptr->next;
  }
  cur_proc->state = ZOMBIE;
  cur_proc->exit_code = exit_code;
  // struct thread_t *sex = cur_proc->queuewaitingforexit;
  // while (sex != NULL) {
  //   sex = pop_from_list(&cur_proc->queuewaitingforexit);
  //   ThreadReady(sex);
  // }
  exit_thread();
}
void ThreadBlock(struct thread_t *whichqueue) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  cpu->cur_thread->state = BLOCKED;
  push_into_list(&whichqueue, cpu->cur_thread);
}
void ThreadReady(struct thread_t *thread) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  thread->state = READYING;
  push_into_list(&cpu->run_queue, thread);
}
void create_kthread(uint64_t entry, struct process_t *proc, uint64_t tid) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  struct thread_t *newthread = create_thread();
  newthread->proc = proc;
  newthread->tid = tid;
  refcount_inc(&proc->cnt);
  uint64_t kstack = (uint64_t)(kmalloc(KSTACKSIZE) + KSTACKSIZE);
  struct StackFrame hh = arch_create_frame(false, entry, kstack - 8);
  newthread->kernel_stack_base = kstack;
  newthread->kernel_stack_ptr = kstack;
  newthread->arch_data.frame = hh;
  refcount_inc(&newthread->count);
  ThreadReady(newthread);
}
void build_fpu_state(void *area) {
  struct fpu_state *state = area;
  state->fcw |= 0x33F;
  state->mxcsr |= 0x1F80;
}
struct thread_t *create_uthread(uint64_t entry, struct process_t *proc,
                                uint64_t tid) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  struct thread_t *newthread = create_thread();
  newthread->proc = proc;
  newthread->tid = tid;
  newthread->fpu_state = (void *)align_up(
      (uint64_t)kmalloc(align_up(get_fpu_storage_size(), 0x1000) + 64), 64);
  kprintf("fpu state %p\r\n", (void *)newthread->fpu_state);
  build_fpu_state(newthread->fpu_state);
  refcount_inc(&proc->cnt);
  uint64_t kstack = (uint64_t)(kmalloc(KSTACKSIZE) + KSTACKSIZE);
  kprintf("user thread creation(): giving %lu for the stack\r\n",
          (size_t)USTACKSIZE);
  uint64_t ustack =
      (uint64_t)uvmm_region_alloc(proc->cur_map, USTACKSIZE, 0) + USTACKSIZE;
  struct StackFrame hh = arch_create_frame(true, entry, ustack - 8);
  newthread->kernel_stack_base = kstack;
  newthread->kernel_stack_ptr = kstack;
  build_fpu_state(newthread->fpu_state);
  newthread->arch_data.frame = hh;
  refcount_inc(&newthread->count);
  return newthread;
}
void clear_and_prepare_thread(struct thread_t *t) {
  t->kernel_stack_ptr = t->kernel_stack_base;
  kfree(t->fpu_state, align_up(get_fpu_storage_size(), 0x1000) + 64);
  t->fpu_state = (void *)align_up(
      (uint64_t)kmalloc(align_up(get_fpu_storage_size(), 0x1000) + 64), 64);
  build_fpu_state(t->fpu_state);
  uint64_t ustack =
      (uint64_t)uvmm_region_alloc(t->proc->cur_map, USTACKSIZE, 0) + USTACKSIZE;
  struct StackFrame hh = arch_create_frame(true, 0, ustack - 8);
  t->arch_data.frame = hh;
}

extern void shitfuck();
void create_kentry() {
  hashmap_set_allocator(kmalloc, kfree);
#if defined(__x86_64__)
  struct process_t *kernelprocess = create_process(&ker_map);

  create_kthread((uint64_t)kentry, kernelprocess, 1);
  create_kthread((uint64_t)reaper, kernelprocess, 0);

  // create_kthread((uint64_t)klocktest, kernelprocess, 2);
  // create_kthread((uint64_t)klocktest2, kernelprocess, 3);
#endif
}
void attach_child(struct process_t *from, struct process_t *child) {
  if (from->children != NULL) {
    struct process_t *lastchild = from->children;
    while (lastchild->children_next) {
      lastchild = lastchild->children_next;
    }
    lastchild->children_next = child;
  } else {
    from->children = child;
  }
}
void start_init() {
  struct process_t *aa = get_process_start();
  struct process_t *FUCKYOU = create_process(aa->cur_map);
  FUCKYOU->parent = NULL; // do NOT link it to the kernel proc
  aa->children = NULL;

  get_process_finish(aa);
  struct thread_t *fun = create_uthread(0, FUCKYOU, 2);
  pagemap *curpagemap = fun->proc->cur_map;
  duplicate_process_fd(aa, FUCKYOU);
  load_elf(curpagemap, "/bin/bash", (char *[]){"/bin/bash", NULL},
           (char *[]){"TERM=linux", NULL}, &fun->arch_data.frame, NULL);
  ThreadReady(fun);
}
int scheduler_fork() {
  struct process_t *oldprocess = get_process_start();
  pagemap *new = new_pagemap();
  struct process_t *newprocess = create_process(new);
  duplicate_pagemap(oldprocess->cur_map, new);
  newprocess->cur_map = new;
  newprocess->cwd = oldprocess->cwd;
  // so fucking simple
  // so fucking easy
  newprocess->parent = oldprocess;
  attach_child(oldprocess, newprocess);
  if (oldprocess->cwdpath)
    newprocess->cwdpath = strdup(oldprocess->cwdpath);
  duplicate_process_fd(oldprocess, newprocess);
  struct thread_t *calledby = arch_get_per_cpu_data()->cur_thread;
  struct thread_t *fun = create_thread();
  fun->fpu_state = (void *)align_up(
      (uint64_t)kmalloc(align_up(get_fpu_storage_size(), 0x1000) + 64), 64);
  memcpy(fun->fpu_state, calledby->fpu_state, get_fpu_storage_size());
  fun->count = 1;
  fun->tid = calledby->tid + 1;
  uint64_t kstack = (uint64_t)(kmalloc(KSTACKSIZE) + KSTACKSIZE);
  fun->kernel_stack_base = kstack;
  fun->kernel_stack_ptr = kstack;
  fun->proc = newprocess;
#if defined __x86_64__
  struct SyscallFrame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rcx, rbx, pad;
  };
  struct SyscallFrame *syscallframe =
      (struct SyscallFrame *)(arch_get_per_cpu_data()
                                  ->arch_data.kernel_stack_ptr -
                              sizeof(struct SyscallFrame));

  fun->arch_data.frame = (struct StackFrame){};
  struct StackFrame *destframe = &fun->arch_data.frame;
  destframe->r15 = syscallframe->r15;
  destframe->r14 = syscallframe->r14;
  destframe->r13 = syscallframe->r13;
  destframe->r12 = syscallframe->r12;
  destframe->r11 = syscallframe->r11;
  destframe->r10 = syscallframe->r10;
  destframe->r9 = syscallframe->r9;
  destframe->r8 = syscallframe->r8;
  destframe->rdi = syscallframe->rdi;
  destframe->rsi = syscallframe->rsi;
  destframe->rbp = syscallframe->rbp;
  destframe->rcx = syscallframe->rcx;
  destframe->rbx = syscallframe->rbx;
  destframe->rip = syscallframe->rcx;
  // USER CODE
  destframe->cs = 0x40 | 3;
  // USER DATA
  destframe->ss = 0x38 | 3;
  destframe->rflags = syscallframe->r11;
  destframe->rsp = arch_get_per_cpu_data()->arch_data.syscall_stack_ptr_tmp;

  fun->arch_data.frame.rax = 0;
  fun->arch_data.fs_base = calledby->arch_data.fs_base;
  get_process_finish(oldprocess);
#endif
  // refcount_inc(&fun->count);
  ThreadReady(fun);
  return fun->proc->pid;
}
// returns the old thread
struct thread_t *switch_queue(struct per_cpu_data *cpu) {
  if (cpu->cur_thread) {
    struct thread_t *old = cpu->cur_thread;
    if (cpu->cur_thread->state == ZOMBIE || cpu->cur_thread->state == BLOCKED) {
      cpu->cur_thread = pop_from_list(&cpu->run_queue);
      cpu->cur_thread->state = RUNNING;
      return old;
    }

    cpu->cur_thread->state = READY;
    push_into_list(&cpu->run_queue, cpu->cur_thread);
    cpu->cur_thread = pop_from_list(&cpu->run_queue);
    if (cpu->cur_thread->state == READYING) {
      cpu->cur_thread->state = READY;
      push_into_list(&cpu->run_queue, cpu->cur_thread);
      cpu->cur_thread = pop_from_list(&cpu->run_queue);
      cpu->cur_thread->state = RUNNING;
      return old;
    }
    cpu->cur_thread->state = RUNNING;
    return old;
  } else {
    cpu->cur_thread = pop_from_list(&cpu->run_queue);
    cpu->cur_thread->state = RUNNING;
    return NULL;
  }
}
#ifdef __x86_64__
void load_ctx(struct StackFrame *context);
void save_ctx(struct StackFrame *dest, struct StackFrame *src);

#endif

void schedd(struct StackFrame *frame) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  if (frame) {
    //sprintf("sched, %p\r\n", frame);
  }
  if (!cpu) {
    return;
  }
  if (cpu->cur_thread == NULL && cpu->run_queue == NULL) {
    // kprintf("schedd(): no threads to run\r\n");
    return;
  }
  if (cpu->cur_thread) {
#if defined(__x86_64__)
    cpu->cur_thread->arch_data.fs_base = rdmsr(x86_FS_BASE);
#endif
  }
  if (frame) {
    if (frame->cs & 3) /* if usermode */
    {
#if defined(__x86_64__)
      if (cpu->cur_thread) {
        fpu_save(cpu->cur_thread->fpu_state);
      }

#endif
    }
  }

  struct thread_t *old = switch_queue(cpu);
#if defined(__x86_64__)
  if (old != NULL && frame != NULL) {
    save_ctx(&old->arch_data.frame, frame);
  }

  if (old) old->syscall_user_sp = cpu->arch_data.syscall_stack_ptr_tmp;
  cpu->arch_data.syscall_stack_ptr_tmp = cpu->cur_thread->syscall_user_sp;

  cpu->arch_data.kernel_stack_ptr = cpu->cur_thread->kernel_stack_ptr;
  arch_switch_pagemap(cpu->cur_thread->proc->cur_map);
  wrmsr(x86_FS_BASE, cpu->cur_thread->arch_data.fs_base);
  change_rsp0(cpu->cur_thread->kernel_stack_base); // do this unconditally
  if (cpu->cur_thread->arch_data.frame.cs & 3)     /* if usermode */
  {
#if defined(__x86_64__)
    fpu_store(cpu->cur_thread->fpu_state);

    __asm__ volatile("swapgs");
#endif
  }
  send_eoi();
  load_ctx(&cpu->cur_thread->arch_data.frame);
#endif
}
struct process_t *get_process_start() {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  uint64_t flags;
asm volatile("pushfq; cli; pop %0" : "=r"(flags));
cpu->freeshit = (void*)flags;
  spinlock_lock(&cpu->cur_thread->proc->lock);
  return cpu->cur_thread->proc;
}
void get_process_finish(struct process_t *proc) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  uint64_t flags = (uint64_t)cpu->freeshit;
  
  spinlock_unlock(&proc->lock);
if (flags & (1 << 9)) {
    __asm__ volatile ("sti");
  }
}
