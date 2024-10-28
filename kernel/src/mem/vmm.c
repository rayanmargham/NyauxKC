#include "vmm.h"
#include "limine.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "utils/basic.h"
#include <stdint.h>

#define EXECUTEDISABLE (1ul << 63)
#define PRESENT (1ul)
#define RWALLOWED (1ul << 1)
#define USERMODE (1ul << 2)
#define CACHEDISABLE (1ul << 4)
#define PATBIT4096 (1ul << 7)
#define PATBIT2MB (1ul << 12)
#define PAGE2MB (1ul << 7)
#define WRITETHROUGH (1ul << 3)

uint64_t *find_pte_and_allocate(uint64_t *pt, uint64_t virt) {
  uint64_t shift = 48;
  for (int i = 0; i < 4; i++) {
    shift -= 9;
    uint64_t idx = (virt >> shift) & 0x1ff;
    uint64_t *page_table =
        (uint64_t *)((uint64_t)pt + hhdm_request.response->offset);
    if (i == 3) {
      return (uint64_t *)((uint64_t)page_table + idx);
    }
    if (!(page_table[idx] & PRESENT)) {
      uint64_t *guy =
          (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
      page_table[idx] = (uint64_t)guy | PRESENT | RWALLOWED;
      pt = guy;
    } else if (page_table[idx] & PAGE2MB) {
      uint64_t *guy =
          (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
      uint64_t old_phys = page_table[idx] & 0x000ffffffffff000;
      uint64_t old_flags = page_table[idx] & ~0x000ffffffffff000;
      for (int j = 0; j < 512; j++) {
        guy[j] = (old_phys + j * 4096) | (old_flags & ~PAGE2MB);
      }
    } else {
      pt = (uint64_t *)(page_table[idx] & 0x000ffffffffff000);
    }
  }
  return 0;
}
uint64_t *find_pte_and_allocate2mb(uint64_t *pt, uint64_t virt) {
  uint64_t shift = 48;
  for (int i = 0; i < 4; i++) {
    shift -= 9;
    uint64_t idx = (virt >> shift) & 0x1ff;
    uint64_t *page_table =
        (uint64_t *)((uint64_t)pt + hhdm_request.response->offset);
    if (i == 2) {
      return (uint64_t *)((uint64_t)page_table + idx);
    }
    if (!(page_table[idx] & PRESENT)) {
      uint64_t *guy =
          (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
      page_table[idx] = (uint64_t)guy | PRESENT | RWALLOWED;
      pt = guy;
    } else {
      pt = (uint64_t *)(page_table[idx] & 0x000ffffffffff000);
    }
  }
  return 0;
}
uint64_t *find_pte(uint64_t *pt, uint64_t virt) {
  uint64_t shift = 48;
  for (int i = 0; i < 4; i++) {
    shift -= 9;
    uint64_t idx = (virt >> shift) & 0x1ff;
    uint64_t *page_table =
        (uint64_t *)((uint64_t)pt + hhdm_request.response->offset);
    if (i == 2) {
      return (uint64_t *)((uint64_t)page_table + idx);
    }
    if (!(page_table[idx] & PRESENT)) {
      return (uint64_t *)((uint64_t)page_table + idx);
    } else {
      pt = (uint64_t *)(page_table[idx] & 0x000ffffffffff000);
    }
  }
  return 0;
}
void map(uint64_t *pt, uint64_t phys, uint64_t virt, uint64_t flags) {
  uint64_t *f = find_pte_and_allocate(pt, virt);
  *f = phys | flags;
}
void map2mb(uint64_t *pt, uint64_t phys, uint64_t virt, uint64_t flags) {
  uint64_t *f = find_pte_and_allocate2mb(pt, virt);
  *f = (phys & ~0x1ffffful) | flags | PAGE2MB;
}
extern void invlpg(uint64_t virt);
extern void switch_to_pagemap(uint64_t pml4);
void switch_cr3(uint64_t pml4) { switch_to_pagemap(pml4); }
void unmap(uint64_t *pt, uint64_t virt) {
  uint64_t *f = find_pte(pt, virt);
  if (f != NULL) {
    *f = 0;
    invlpg(virt);
  }
}
uint64_t virt_to_phys(uint64_t *pt, uint64_t virt) {
  uint64_t *f = find_pte(pt, virt);
  if (f != NULL) {
    return (uint64_t)f & 0x000ffffffffff000;
  } else {
    return 0;
  }
}
typedef struct {
  uint64_t *pml4;
} pagemap;
pagemap ker_map;
void vmm_init() {
  ker_map.pml4 =
      (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
  uint64_t kernel_size = align_up((uint64_t)THE_REAL, 4096);
  for (uint64_t i = 0; i != kernel_size; i += 4096) {
    map(ker_map.pml4, kernel_address.response->physical_base + i,
        kernel_address.response->virtual_base + i, PRESENT | RWALLOWED);
  }
  kprintf("Kernel Mapped!\n");
  uint64_t hddm_pages = 0;
  for (uint64_t i = 0; i < 0x100000000; i += 2097152) {
    assert(i % 2097152 == 0);
    map2mb(ker_map.pml4, i, hhdm_request.response->offset + i,
           PRESENT | RWALLOWED);
    hddm_pages += 1;
  }
  kprintf("Above 4Gib Mapped! Mapping Memory Map!\n");
  for (uint64_t i = 0; i != memmap_request.response->entry_count; i += 1) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    switch (entry->type) {
    case LIMINE_MEMMAP_FRAMEBUFFER:
      uint64_t disalign = entry->base % 2097152;
      entry->base = align_down(entry->base, 2097152);
      uint64_t page_amount =
          align_up(entry->length - disalign, 2097152) / 2097152;
      for (uint64_t j = 0; j != page_amount; j++) {
        map2mb(ker_map.pml4, entry->base + (j * 2097152),
               hhdm_request.response->offset + entry->base + (j * 2097152),
               PRESENT | RWALLOWED | WRITETHROUGH | PATBIT2MB);
      }
      hddm_pages += page_amount;
      break;
    default:
      uint64_t disalignz = entry->base % 2097152;
      entry->base = align_down(entry->base, 2097152);
      uint64_t page_amountz =
          align_up(entry->length - disalignz, 2097152) / 2097152;
      for (uint64_t j = 0; j != page_amountz; j++) {
        map2mb(ker_map.pml4, entry->base + (j * 2097152),
               hhdm_request.response->offset + entry->base + (j * 2097152),
               PRESENT | RWALLOWED);
      }
      hddm_pages += page_amount;
      break;
    }
  }
  kprintf("HDDM Pages %d\n", hddm_pages);
  // panic("h");
  switch_cr3((uint64_t)(ker_map.pml4));
  kprintf("Kernel is now in its own pagemap :)\n");
}