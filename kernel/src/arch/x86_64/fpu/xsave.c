#include "xsave.h"
#include <stdint.h>
void (*fpu_save)(void *area);
void (*fpu_store)(void *area);
uint64_t fpu_storage_size;
static inline void xsave(void *area) {
  asm volatile("xsave (%0)"
               :
               : "r"(area), "a"(0xFFFF'FFFF), "d"(0xFFFF'FFFF)
               : "memory");
}

static inline void xrstor(void *area) {
  asm volatile("xrstor (%0)"
               :
               : "r"(area), "a"(0xFFFF'FFFF), "d"(0xFFFF'FFFF)
               : "memory");
}

static inline void fxsave(void *area) {
  asm volatile("fxsave (%0)" : : "r"(area) : "memory");
}

static inline void fxrstor(void *area) {
  asm volatile("fxrstor (%0)" : : "r"(area) : "memory");
}
uint64_t get_fpu_storage_size() { return fpu_storage_size; }
// on cpu init
void cpu_fpu_init(bool xsave) {
  uint64_t cr4 = cr4_read();
  cr4 |= (1 << 9);  // enable fxstor and fxsav
  cr4 |= (1 << 10); // Setting this flag implies that the operating system
                    // exception handler
  cr4 &= ~(1 << 2); // This action disables emulation of the x87 FPU
  cr4 |= (1 << 1); // This setting is required for Intel 64 and IA-32 processors
  cr4_write(cr4);
  if (xsave) {
    cr4 = cr4_read();
    cr4 |= (1 << 18); // enable xsave feature set
    cr4_write(cr4);
    uint64_t xcr0 = 0;
    xcr0 |= (1 << 0);
    xcr0 |= (1 << 1);
    uint32_t eax, ebx, ecx, edx;
    cpuid(0xd, 0, &eax, &ebx, &ecx, &edx);
    if (eax & (1 << 2)) {
      xcr0 |= (1 << 2);
    }
    if (eax & (0b11 << 3)) {
      xcr0 |= (0b11 << 3);
    }
    if (eax & (0b111 << 5)) {
      xcr0 |= (0b111 << 5);
    }
    if (eax & (1 << 9)) {
      xcr0 |= (1 << 9);
    }
    if (eax & (0b11 << 17)) {
      xcr0 |= (0b11 << 17);
    }
    asm volatile("xsetbv" : : "a"(xcr0), "d"(xcr0 >> 32), "c"(0) : "memory");
  }
}
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
    if (fpu_storage_size == 0) {
      fpu_storage_size = ecx;
      fpu_save = xsave;
      fpu_store = xrstor;
    }

    cpu_fpu_init(true);

  } else {
    kprintf("arch_fpu(): using legacy fxstor\r\n");
    // panic("nyaux needs xsave :)\r\n");
    // fxstor fxsave, 512 byte save
    if (fpu_storage_size) {
      fpu_storage_size = 512;
      fpu_save = fxsave;
      fpu_store = fxrstor;
    }
    cpu_fpu_init(false);
  }
}