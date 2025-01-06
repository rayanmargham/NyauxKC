#include <stdbool.h>
#include <stdint.h>
#include "utils/basic.h"
#include "sched/sched.h"
struct KMutex {
	spinlock_t protectionlock;
	bool lock;
	struct thread_t *blockedthreadsqueue;
};
void release_kmutex(struct KMutex* mutex);
void acquire_kmutex(struct KMutex* mutex);