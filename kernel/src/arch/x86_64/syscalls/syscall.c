#include "syscall.h"
#include "../instructions/instructions.h"
#include "utils/basic.h"
#include <stdint.h>

void syscall_exit() { panic("unimpl'd"); }
void syscall_debug(char *string, size_t length) { panic("unimpl'd"); }
extern void syscall_entry();

void syscall_init() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 11)) {
    uint64_t IA_32_STAR = 0;
    IA_32_STAR |= ((uint64_t)0x28 << 32);
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