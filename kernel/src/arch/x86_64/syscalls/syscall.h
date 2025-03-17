#pragma once
#include "arch/x86_64/cpu/structures.h"
#include <stddef.h>
void RegisterSyscall(void (*ptr)(struct StackFrame *frame), size_t offset);
void syscall_init();
struct __syscall_ret {
  uint64_t ret;
  uint64_t errno;
};