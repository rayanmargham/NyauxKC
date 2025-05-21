#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
extern volatile struct limine_hhdm_request hhdm_request;
extern void outb(uint16_t port, uint8_t data);
extern void outw(uint16_t port, uint16_t data);
extern void outd(uint16_t port, uint32_t data);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t ind(uint16_t port);
static inline uint64_t rdmsr(uint32_t msr) {
  uint32_t low = 0;
  uint32_t hi = 0;
  __asm__ volatile("rdmsr" : "=a"(low), "=d"(hi) : "c"(msr));
  uint64_t combined = ((uint64_t)hi << 32) | low;
  return combined;
}
static inline void wrmsr(uint32_t msr, uint64_t value) {
  uint32_t low = (uint32_t)value;
  uint32_t hi = (uint32_t)(value >> 32);
  __asm__ volatile("wrmsr" : : "a"(low), "d"(hi), "c"(msr));
}

static inline uint64_t get_lapic_address() {
  uint64_t addr = (rdmsr(0x1b) & 0xfffff000);
  return addr;
}
static inline uint32_t get_lapic_id() {
  volatile uint64_t addr = (rdmsr(0x1b) & 0xfffff000);

  volatile uint32_t *ptr =
      (volatile uint32_t *)(addr + hhdm_request.response->offset);
  volatile uint32_t val = *(volatile uint32_t *)((uint64_t)ptr + 0x20);

  return (val >> 24);
}
static inline uint64_t cr4_read() {
  uint64_t value;
  asm volatile("movq %%cr4, %0" : "=r"(value));
  return value;
}
static inline void cr4_write(uint64_t value) {
  asm volatile("movq %0, %%cr4" : : "r"(value) : "memory");
}
static inline bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax,
                         uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
  uint32_t cpuid_max;
  asm volatile("cpuid"
               : "=a"(cpuid_max)
               : "a"(leaf & 0x80000000)
               : "rbx", "rcx", "rdx");
  if (leaf > cpuid_max) {
    return false;
  }
  asm volatile("cpuid"
               : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
               : "a"(leaf), "c"(subleaf));
  return true;
}
#ifdef __cplusplus
}
#endif