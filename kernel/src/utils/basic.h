#pragma once
#include <stdint.h>
#include <term/term.h>
static void hcf(void) {
  for (;;) {
#if defined(__x86_64__)
    asm("hlt");
#elif defined(__aarch64__) || defined(__riscv)
    asm("wfi");
#elif defined(__loongarch64)
    asm("idle 0");
#endif
  }
}
#define assert(expression)                                                     \
  do {                                                                         \
    if (!(expression)) {                                                       \
      kprintf("Assertion failed in function %s. File: %s, line %d. %s\n",      \
              __func__, __FILE__, __LINE__, #expression);                      \
      panic("Assertion Failed :c");                                            \
    }                                                                          \
  } while (0)
static inline void panic(char *msg) {
  kprintf("NYAUX Panic! Reason: ");
  kprintf("%s\n", msg);
  kprintf("oopsie uwu :3\n");
  hcf();
}

#define SPINLOCK_INITIALIZER 0

typedef int spinlock_t;

static inline void spinlock_lock(spinlock_t *lock) {
  while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
#if defined(__x86_64__)
    __asm__ volatile("pause");
#endif
  }
}

static inline void spinlock_unlock(spinlock_t *lock) {
  __sync_bool_compare_and_swap(lock, 1, 0);
}

typedef enum { OKAY, ERR } result_type;

typedef struct {
  result_type type;
  union {
    bool okay;
    char *err_msg;
  };
} result;

static inline void unwrap_or_panic(result res) {
  if (res.type == OKAY) {
    return;
  } else {
    kprintf("Result Throw'd an error: %s\n", res.err_msg);
    panic("unwrap_or_panic() failed...");
  }
}
static inline uint32_t align_up(uint32_t value, uint32_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static inline uint32_t align_down(uint32_t value, uint32_t alignment) {
  return value & ~(alignment - 1);
}
// stolen :)
static inline uint64_t next_pow2(uint64_t x) {
#ifdef __GNUC__
  return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
#else
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x;
#endif
}