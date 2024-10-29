#include "kmem.h"

void *kmalloc(uint64_t amount) {
  if (amount > 1024) {
    return kvmm_region_alloc(amount, PRESENT | RWALLOWED);
  } else {
    return slaballocate(amount);
  }
}
void kfree(void *addr, uint64_t size) {
  if (size > 1024) {
    return kvmm_region_dealloc(addr);
  } else {
    return slabfree(addr);
  }
}