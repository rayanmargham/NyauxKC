#pragma once
#include <limine.h>
#include <stdint.h>
#include <utils/basic.h>
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern char THE_REAL[];
extern volatile struct limine_executable_address_request kernel_address;
void vmm_init();
void *kvmm_region_alloc(uint64_t amount, uint64_t flags);
void kvmm_region_dealloc(void *addr);
uint64_t kvmm_region_bytesused();
#define EXECUTEDISABLE (1ul << 63)
#define PRESENT (1ul)
#define RWALLOWED (1ul << 1)
#define USERMODE (1ul << 2)
#define CACHEDISABLE (1ul << 4)
#define PATBIT4096 (1ul << 7)
#define PATBIT2MB (1ul << 12)
#define PAGE2MB (1ul << 7)
#define WRITETHROUGH (1ul << 3)
extern void *memset(void *s, int c, size_t n);
void per_cpu_vmm_init();
