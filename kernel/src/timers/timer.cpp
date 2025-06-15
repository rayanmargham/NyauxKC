#include "arch/arch.h"

#include <timers/hpet.hpp>
#include <timers/pvclock.hpp>
#include <timers/timer.hpp>
void *Timer = nullptr;
struct nyaux_kernel_info info {
  .timestamp = 0,
  .lock = {0},
  .cmdarray = NULL,
};
extern "C" {
  bool GenericTimerActive() {
    if (Timer == nullptr) {
      return false;
    } else {
      return true;
    }
  }
  void GenericTimerInit() {
    if ((!arch_check_kvm_clock()) && !arch_check_can_pvclock()) {
      Timer = static_cast<void *>(new HPET);
    } else {
      Timer = static_cast<void *>(new pvclock);
    }
    info.timestamp = limine_boot_time.response->boot_time;
  }
  int GenericTimerStallPollps(size_t ps) {
    if (!Timer) {
      return -1;
    }
    return static_cast<GenericTimer *>(Timer)->stall_poll_ps(ps);
  }
  int GenericTimerStallPolfs(size_t fs) {
    if (!Timer) {
      return -1;
    }
    return static_cast<GenericTimer *>(Timer)->stall_poll_fs(fs);
  }
  int GenericTimerStallPollms(size_t ms) {
    if (!Timer) {
      return -1;
    }
    return static_cast<GenericTimer *>(Timer)->stall_poll_ms(ms);
  }
  int GenericTimerStallPollus(size_t us) {
    if (!Timer) {
      return -1;
    }
    return static_cast<GenericTimer *>(Timer)->stall_poll_us(us);
  }
  int GenericTimerStallPollns(size_t ns) {
    if (!Timer) {
      return -1;
    }
    return static_cast<GenericTimer *>(Timer)->stall_poll_ns(ns);
  }
  size_t GenericTimerGetns() {
    if (!Timer) {
      return 0;
    }
    return static_cast<GenericTimer *>(Timer)->get_ns();
  }
  size_t GenericTimerGetms() {
    if (!Timer) {
      return 0;
    }
    return static_cast<GenericTimer *>(Timer)->get_ms();
  }
}
