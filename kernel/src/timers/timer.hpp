#pragma once
#include "Mutexes/seqlock.h"
#include <cppglue/glue.hpp>
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#ifdef __cplusplus
class GenericTimer {

public:
  GenericTimer() {}

  virtual size_t get_ps() { return 0; }; // pico second
  virtual size_t get_fs() { return 0; }  // fento second
  virtual size_t get_ns() { return 0; }  // nanosecond
  virtual size_t get_us() { return 0; }  // microsecond
  virtual size_t get_ms() { return 0; };
  virtual int stall_poll_ps(size_t ps) {
    return -1;
  }; // returns -1 if this cannot be done
  virtual int stall_poll_fs(size_t fs) { return -1; };
  virtual int stall_poll_ns(size_t ns) { return -1; };
  virtual int stall_poll_us(size_t us) { return -1; };
  virtual int stall_poll_ms(size_t ms) { return -1; };
};
#endif

extern void
    *Timer; // ignore this warning, pragma once is protecting us from an ODR
#ifdef __cplusplus
extern "C" {
#endif
struct nyaux_kernel_info {
  __int128_t timestamp;
  struct seq_lock lock;
};
extern struct nyaux_kernel_info info;
extern volatile struct limine_boot_time_request limine_boot_time;
  void GenericTimerInit();
  int GenericTimerStallPollps(size_t ps);
  int GenericTimerStallPolfs(size_t fs);
  int GenericTimerStallPollms(size_t ms);
  int GenericTimerStallPollus(size_t us);
  int GenericTimerStallPollns(size_t ns);
  size_t GenericTimerGetns();
  size_t GenericTimerGetms();

#ifdef __cplusplus
}
#endif
