#pragma once
#include "timers/timer.hpp"
#include <arch/x86_64/instructions/instructions.h>

struct pvclock_vcpu_time_info {
  uint32_t version;
  uint32_t pad0;
  uint64_t tsc_timestamp;
  uint64_t system_time;
  uint32_t tsc_to_system_mul;
  int8_t tsc_shift;
  uint8_t flags;
  uint8_t pad[2];
} __attribute__((packed, aligned(8))); /* 32 bytes */
#ifdef __cplusplus
struct pvclock : GenericTimer {
public:
  size_t get_ps() override;
  size_t get_fs() override;
  size_t get_ns() override;
  size_t get_us() override;
  size_t get_ms() override;
  int stall_poll_ps(size_t) override;
  int stall_poll_fs(size_t) override;
  int stall_poll_ms(size_t ms) override;
  int stall_poll_ns(size_t ns) override;
  int stall_poll_us(size_t us) override;
  pvclock();
};
#endif
