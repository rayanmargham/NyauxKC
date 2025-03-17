#include "syscall.h"
#include "../instructions/instructions.h"
#include "elf/symbols/symbols.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"
#include <stdint.h>

struct __syscall_ret syscall_exit() {
  kprintf("hi FROM syscall_exit()\r\n");
  return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_debug(char *string, size_t length) {
  char *buffer = kmalloc(1024);
  memcpy(buffer, string, length);
  buffer[length] = '\0';
  kprintf("userland: %s", buffer);
  return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_setfsbase(uint64_t ptr) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  cpu->cur_thread->arch_data.fs_base = ptr;
  wrmsr(0xC0000100, cpu->cur_thread->arch_data.fs_base);
  return (struct __syscall_ret){0, 0};
}
extern void syscall_entry();

void syscall_init() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 11)) {
    uint64_t IA_32_STAR = 0;
    IA_32_STAR |= ((uint64_t)0x28 << 32);
    IA_32_STAR |= ((uint64_t)0x30 << 48);
    wrmsr(0xC0000081, IA_32_STAR);
    wrmsr(0xC0000082, (uint64_t)syscall_entry);
    wrmsr(0xC0000084, (1 << 9));
    uint64_t IA_32_EFER = rdmsr(0xC0000080);
    IA_32_EFER |= (1);
    wrmsr(0xC0000080, IA_32_EFER);

  } else {
    panic("the syscall instruction is NOT supported. nyaux needs it to run");
  }
}