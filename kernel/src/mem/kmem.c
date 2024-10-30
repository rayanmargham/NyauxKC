#include "kmem.h"
#include "term/term.h"

void *kmalloc(uint64_t amount) {
  if (amount > 1024) {
    void *him = kvmm_region_alloc(amount, PRESENT | RWALLOWED);
    kprintf("Allocated with base: %p\n", him);
    return him;
  } else {
    void *him = slaballocate(amount);
    kprintf("Allocated slab with base: %p\n", him);
    return him;
  }
}
void kfree(void *addr, uint64_t size) {
  if (size > 1024) {
    kprintf("deallocing vmm region address of base: %p\n", addr);
    kvmm_region_dealloc(addr);
  } else {
    slabfree(addr);
  }
}
