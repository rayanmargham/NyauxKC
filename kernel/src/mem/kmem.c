#include "kmem.h"
#include "term/term.h"

void *kmalloc(uint64_t amount) {
  if (amount > 1024) {
    void *him = kvmm_region_alloc(amount, PRESENT | RWALLOWED);
    return him;
  } else {
    void *him = slaballocate(amount);
    return him;
  }
}
void kfree(void *addr, uint64_t size) {
  if (size > 1024) {
    kvmm_region_dealloc(addr);
  } else {
    slabfree(addr);
  }
}
