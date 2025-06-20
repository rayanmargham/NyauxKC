#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <limine.h>
#include <stdint.h>
#include <utils/basic.h>
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern char THE_REAL[];
extern volatile struct limine_executable_address_request kernel_address;
void vmm_init();

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
#include <utils/basic.h>
extern void *memset(void *s, int c, size_t n);

void per_cpu_vmm_init();
typedef struct _VMMRegion {
  uint64_t base;
  uint64_t length;
  bool nocopy;
  bool demand_paged;
  struct _VMMRegion *next;
} VMMRegion;
typedef struct {
  uint64_t *root;
  VMMRegion *head;
  VMMRegion *userhead;
} pagemap;
extern pagemap ker_map;

void *kvmm_region_alloc(pagemap *map, uint64_t amount, uint64_t flags);
void kvmm_region_dealloc(pagemap *map, void *addr);
void *uvmm_region_alloc(pagemap *map, uint64_t amount, uint64_t flags);
void *uvmm_region_alloc_fixed(pagemap *map, uint64_t virt, size_t size,
                              bool force);
void uvmm_region_dealloc(pagemap *map, void *addr);

pagemap *new_pagemap();
void kprintf_all_vmm_regions();
void duplicate_pagemap(pagemap *maptoduplicatefrom, pagemap *to);
void deallocate_all_user_regions(pagemap *target);
void free_pagemap(pagemap *take);
bool iswithinvmmregion(pagemap *map, uint64_t virt);
void *uvmm_region_alloc_demend_paged(pagemap *map, uint64_t amount);
void unmap_range(pagemap *take, void *addr, size_t sizeinbytes);
#ifdef __cplusplus
}
#endif