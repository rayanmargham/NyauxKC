#pragma once
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <stdint.h>
void x86_64_init_pagemap(pagemap *take);
void x86_64_destroy_pagemap(pagemap *take);
uint64_t x86_64_map_kernelhhdmandmemorymap(pagemap *take);
void x86_64_map_vmm_region(pagemap *take, uint64_t base,
                           uint64_t length_in_bytes);
void x86_64_unmap_vmm_region(pagemap *take, uint64_t base,
                             uint64_t length_in_bytes);
void x86_64_switch_pagemap(pagemap *take);
void x86_64_map_vmm_region_user(pagemap *take, uint64_t base,
                                uint64_t length_in_bytes);
uint64_t x86_64_get_phys(pagemap *take, uint64_t virt);
#define EXECUTEDISABLE (1ul << 63)
#define PRESENT (1ul)
#define RWALLOWED (1ul << 1)
#define USERMODE (1ul << 2)
#define CACHEDISABLE (1ul << 4)
#define PATBIT4096 (1ul << 7)
#define PATBIT2MB (1ul << 12)
#define PAGE2MB (1ul << 7)
#define WRITETHROUGH (1ul << 3)
#define PAGESIZE 4096
