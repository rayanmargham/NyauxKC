#include "kmem.h"
#include "term/term.h"

void *kmalloc(uint64_t amount) {
  if (amount > 1024) {

    void *him = kvmm_region_alloc(amount, PRESENT | RWALLOWED);
    return him;
  } else {
#ifdef __SANITIZE_ADDRESS__
    void *him = slaballocate(amount + 256);
    memset(him + amount, 0xFD, 256);
    return him;

#else
    void *him = slaballocate(amount);
    return him;
#endif
  }
}
void kfree(void *addr, uint64_t size) {
  if (size >> 63) {
    kprintf("kfree: memory corruption detected\n");
    __builtin_trap();
  }
  if (size > 1024) {
    kvmm_region_dealloc(addr);
  } else {
    slabfree(addr);
  }
}
