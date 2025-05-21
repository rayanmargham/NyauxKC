#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "sched/sched.h"
#include "utils/basic.h"
#include <stdbool.h>
#include <stdint.h>
struct KMutex {
  spinlock_t protectionlock;
  bool lock;
  struct thread_t *blockedthreadsqueue;
};
void release_kmutex(struct KMutex *mutex);
void acquire_kmutex(struct KMutex *mutex);
#ifdef __cplusplus
}
#endif