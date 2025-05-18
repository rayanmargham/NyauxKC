#include "KMutex.h"

#include <stdbool.h>

#include "sched/sched.h"
#include "utils/basic.h"

void acquire_kmutex(struct KMutex* mutex)
{
	spinlock_lock(&mutex->protectionlock);
	// kprintf(__func__ "(): lock is %d\n", mutex->lock);
	if (!mutex->lock)
	{
		mutex->lock = true;
		spinlock_unlock(&mutex->protectionlock);
		return;
	}
	else
	{
		ThreadBlock(mutex->blockedthreadsqueue);
		spinlock_unlock(&mutex->protectionlock);
		sched_yield();
	}
}
void release_kmutex(struct KMutex* mutex)
{
	spinlock_lock(&mutex->protectionlock);
	if (mutex->blockedthreadsqueue == NULL)
	{
		mutex->lock = false;
		spinlock_unlock(&mutex->protectionlock);
		return;
	}
	else
	{
		// kprintf("hes\n");
		hcf();
		struct thread_t* thread = pop_from_list(&mutex->blockedthreadsqueue);
		ThreadReady(thread);
		spinlock_unlock(&mutex->protectionlock);
	}
}
