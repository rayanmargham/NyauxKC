#include "pvclock.hpp"
#include "arch/x86_64/instructions/instructions.h"
#include "mem/pmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include <stdint.h>

pvclock::pvclock() {
  // as pvclock is a x86_64 only feature we can assume shit here so its fine :3
  // verify we actually HAVE tsc
  uint32_t eax, edx, ebx, ecx;
  cpuid(1, 0, &eax, &ebx, &ecx, &edx);
  if (!(edx & (1 << 4))) {
    panic("okay so no tsc = die");
  }
  // pvclock is per cpu :c
  struct per_cpu_data *vcpu = arch_get_per_cpu_data();
  // listen, kvm wants a full page. because of the way it handles it. so heres
  // what we do, directly call pmm alloc
  uint64_t addr_of_page = (uint64_t)pmm_alloc();
  memset((void *)(addr_of_page), 0, 4096);
  vcpu->arch_data.pvclock = (struct pvclock_vcpu_time_info *)(addr_of_page);

  wrmsr(0x4b564d01,
        ((addr_of_page - (uint64_t)hhdm_request.response->offset) | 1));
  kprintf_log(TRACE, "pvclock has now inited lol\r\n");
}
size_t pvclock::get_ms() { return get_ns() / 1000000; }

size_t pvclock::get_ps() { return 0; }
size_t pvclock::get_fs() { return 0; }
size_t pvclock::get_ns() {
  struct per_cpu_data *vcpu = arch_get_per_cpu_data();
  struct pvclock_vcpu_time_info *info = vcpu->arch_data.pvclock;
  uint32_t version = __atomic_load_n(&info->version, __ATOMIC_ACQUIRE);
  uint64_t tsc_value, tsc_timestamp, system_time;
  uint32_t tsc_to_system_mul;
  int8_t shift;
  for (;;) {
    if ((version & 1) == 0) {
      tsc_timestamp = info->tsc_timestamp;
      system_time = info->system_time;
      tsc_to_system_mul = info->tsc_to_system_mul;
      shift = info->tsc_shift;
      tsc_value = rdtsc();
    }
    uint32_t new_version = __atomic_load_n(&info->version, __ATOMIC_ACQUIRE);
    if (((version & 1) == 0) && new_version == version) {
      break;
    }
    version = new_version;
  }
  size_t time = tsc_value - tsc_timestamp;
  if (shift >= 0) {
    time = time << shift;
  } else {
    time = time >> -shift;
  }
  time = ((__uint128_t)time * tsc_to_system_mul) >> 32;
  time = time + system_time;
  return time;
}
size_t pvclock::get_us() { return get_ns() / 1000; }
int pvclock::stall_poll_fs(size_t) { return -1; }
int pvclock::stall_poll_ps(size_t) { return -1; }
int pvclock::stall_poll_us(size_t us) {
  size_t poll_start = get_ns();
  while ((get_ns() - poll_start) < us * 1000) {
  }
  return 0;
}
int pvclock::stall_poll_ms(size_t ms) {
  size_t poll_start = get_ns();
  while ((get_ns() - poll_start) < ms * 1000000) {
  }
  return 0;
}
int pvclock::stall_poll_ns(size_t ns) {
  size_t poll_start = get_ns();
  while ((get_ns() - poll_start) < ns) {
  }
  return 0;
}
