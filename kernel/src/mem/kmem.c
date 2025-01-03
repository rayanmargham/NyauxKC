#include "kmem.h"

#include <stdint.h>

#include "term/term.h"
#include "utils/basic.h"
spinlock_t mem_lock;
void* kmalloc(uint64_t amount)
{
	spinlock_lock(&mem_lock);
	if (amount > 1024)
	{
		void* him = kvmm_region_alloc(amount, PRESENT | RWALLOWED);
		memset(him, 0, amount);
		spinlock_unlock(&mem_lock);
		return him;
	}
	else
	{
#ifdef __SANITIZE_ADDRESS__
		void* him = slaballocate(amount + 256);
		memset(him + amount, 0xFD, 256);
		spinlock_unlock(&mem_lock);
		return him;

#else
		void* him = slaballocate(amount);
		memset(him, 0, amount);
		spinlock_unlock(&mem_lock);
		return him;
#endif
	}
}
void kfree(void* addr, uint64_t size)
{
	spinlock_lock(&mem_lock);
	if (size >> 63)
	{
		kprintf("kfree: memory corruption detected\r\n");
		spinlock_unlock(&mem_lock);
		__builtin_trap();
	}
	if (size > 1024)
	{
		kvmm_region_dealloc(addr);
		spinlock_unlock(&mem_lock);
	}
	else
	{
		slabfree(addr);
		spinlock_unlock(&mem_lock);
	}
}
