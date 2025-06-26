#include "kmem.h"

#include <stdint.h>

#include "mem/vmm.h"
#include "term/term.h"
#include "utils/basic.h"
spinlock_t mem_lock;
void *kmalloc(uint64_t amount) {
// it is fine that slab allocate only has 8 byte alignment
// sse shit only cares about 16 byte alignment, everything else doesnt mind
uint64_t flags;
  // store the old rflags, disable interrupts and do our print
  asm volatile("pushfq; cli; pop %0" : "=r"(flags));
  spinlock_lock(&mem_lock);
  if (amount > 1024) {
    void *him = kvmm_region_alloc(&ker_map, amount, PRESENT | RWALLOWED);
    memset(him, 0, amount);
    spinlock_unlock(&mem_lock);
 if (flags & 1 << 9) {
    asm volatile("sti");
  }
    return him;
  } else {
#ifdef __SANITIZE_ADDRESS__
    void *him = slaballocate(amount + 256);
    memset(him + amount, 0xFD, 256);
    spinlock_unlock(&mem_lock);
    if (flags & 1 << 9) {
    asm volatile("sti");
  }
    return him;

#else
    void *him = slaballocate(amount);
    memset(him, 0, amount);
spinlock_unlock(&mem_lock);
 if (flags & 1 << 9) {
    asm volatile("sti");
  }
    return him;
#endif
  }
}
void kfree(void *addr, uint64_t size) {
uint64_t flags;
  // store the old rflags, disable interrupts and do our print
  asm volatile("pushfq; cli; pop %0" : "=r"(flags));
  spinlock_lock(&mem_lock);
  if (size >> 63) {
    kprintf("kfree: memory corruption detected\r\n");
    spinlock_unlock(&mem_lock);

    __builtin_trap();
  }
  if (size > 1024) {
    kvmm_region_dealloc(&ker_map, addr);
    spinlock_unlock(&mem_lock);
if (flags & 1 << 9) {
    asm volatile("sti");
  }
  } else {
    slabfree(addr);
    spinlock_unlock(&mem_lock);
 if (flags & 1 << 9) {
    asm volatile("sti");
  }
  }
}
