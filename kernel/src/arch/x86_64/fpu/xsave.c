#include "xsave.h"
#include "utils/basic.h"

void (*fpu_save)();
void (*fpu_store)();
uint64_t fpu_storage_size;
void fpu_init() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  if (ecx & ((uint32_t)1 << 26)) {
    kprintf("arch_fpu(): xsave support found\r\n");
    eax = 0;
    ebx = 0;
    ecx = 0;
    edx = 0;
    cpuid(0xd, 0, &eax, &ebx, &ecx, &edx);
    kprintf("arch_fpu(): xsave size is %d\r\n", ecx);
    fpu_storage_size = ecx;
  } else {
    kprintf("arch_fpu(): using legacy fxstor\r\n");
    // panic("nyaux needs xsave :)\r\n");
    // fxstor fxsave, 512 byte save
    fpu_storage_size = 512;
  }
}