#include "pt.h"
#include "mem/vmm.h"
#include "term/term.h"
#include "utils/basic.h"
#include <stdint.h>

uint64_t *find_pte_and_allocate(uint64_t *pt, uint64_t virt) {
  uint64_t shift = 48;
  for (int i = 0; i < 4; i++) {
    shift -= 9;
    uint64_t idx = (virt >> shift) & 0x1ff;
    uint64_t *page_table =
        (uint64_t *)((uint64_t)pt + hhdm_request.response->offset);
    if (i == 3) {
      return page_table + idx;
    }
    if (!(page_table[idx] & PRESENT)) {
      uint64_t *guy =
          (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
      page_table[idx] = (uint64_t)guy | PRESENT | RWALLOWED;
      pt = guy;
    } else if (page_table[idx] & PAGE2MB) {
      uint64_t *guy = (uint64_t *)((uint64_t)pmm_alloc());
      uint64_t old_phys = page_table[idx] & 0x000ffffffffff000;
      uint64_t old_flags = page_table[idx] & ~0x000ffffffffff000;
      for (int j = 0; j < 512; j++) {
        guy[j] = (old_phys + j * 4096) | (old_flags & ~PAGE2MB);
      }
      pt = (uint64_t *)((uint64_t)guy - hhdm_request.response->offset);
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
      return page_table + idx;
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

    /* If any entry is not present then there is no mapping
       break and return NULL; */
    if (!(page_table[idx] & PRESENT))
      break;

    /* Is the Page Size bit set? */
    if (page_table[idx] & PAGE2MB) {
      /* If the Page Size bit is set on a PML4 entry it's an error
         break and return NULL. If Page Size bit is set on a PDPT entry
         print warning and return the PDPT entry */
      if (i == 0)
        panic("find_pte() error: LargePageSize bit set in PML4 entry");
      else if (i == 1)
        kprintf("find_pte() warning: Found 1GiB page PDPT entry");

      /* We have reached a valid entry that is present with Page Size
         bit set. We are finished, don't descend further */
      return page_table + idx;
    } else {
      /* If we have reached the page table then we have found
         a page table entry, return it */

      if (i == 3)
        return page_table + idx;

      pt = (uint64_t *)(page_table[idx] & 0x000ffffffffff000);
    }
  }
  return NULL;
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
uint64_t pte_to_phys(uint64_t pte) { return pte & 0x000ffffffffff000; }
void x86_64_switch_pagemap(pagemap *take) {
  assert(take != NULL);
  kprintf("Giving Address %p\n", (uint64_t *)(take->root));
  // kprintf("u were about to tripple fault \n");

  // hcf();
  switch_to_pagemap((uint64_t)(take->root));
}
uint64_t unmap(uint64_t *pt, uint64_t virt) {
  uint64_t *f = find_pte(pt, virt);
  if (f != NULL) {
    uint64_t old_phys = pte_to_phys(*f);
    *f = 0;
    invlpg(virt);
    return old_phys;
  } else {
    return 0;
  }
}

void x86_64_map_vmm_region(pagemap *take, uint64_t base,
                           uint64_t length_in_bytes) {
  uint64_t length_in_4kib = length_in_bytes / 4096;
  for (uint64_t i = 0; i != length_in_4kib; i++) {
    void *page =
        (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
    map(take->root, (uint64_t)page, base + (i * 4096), PRESENT | RWALLOWED);
  }
}
void x86_64_unmap_vmm_region(pagemap *take, uint64_t base,
                             uint64_t length_in_bytes) {
  uint64_t amount_to_allocateinpages = length_in_bytes / 4096;
  for (uint64_t i = 0; i != amount_to_allocateinpages; i++) {
    uint64_t phys = unmap(take->root, base + (i * 4096));
    pmm_dealloc((void *)(phys + hhdm_request.response->offset));
  }
}
void x86_64_destroy_pagemap(pagemap *take) {
  // TODO
}

uint64_t x86_64_map_kernelhhdmandmemorymap(pagemap *take) {
  uint64_t kernel_size = align_up((uint64_t)THE_REAL, 4096);
  for (uint64_t i = 0; i != kernel_size; i += 4096) {
    map(take->root, kernel_address.response->physical_base + i,
        kernel_address.response->virtual_base + i, PRESENT | RWALLOWED);
  }
  kprintf("vmm(x86_64)(): Kernel Mapped!\n");
  uint64_t hhdm_pages = 0;
  for (uint64_t i = 0; i < 0x100000000; i += MIB(2)) {
    assert(i % MIB(2) == 0);
    map2mb(take->root, i, hhdm_request.response->offset + i,
           PRESENT | RWALLOWED);
    hhdm_pages += 1;
  }
  kprintf("vmm(x86_64)(): Above 4Gib Mapped! Mapping Memory Map!\n");
  for (uint64_t i = 0; i != memmap_request.response->entry_count; i += 1) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    switch (entry->type) {
    case LIMINE_MEMMAP_FRAMEBUFFER: {
      uint64_t disalign = entry->base % MIB(2);
      entry->base = align_down(entry->base, MIB(2));
      uint64_t page_amount =
          align_up(entry->length + disalign, MIB(2)) / MIB(2);
      for (uint64_t j = 0; j != page_amount; j++) {
        map2mb(take->root, entry->base + (j * MIB(2)),
               hhdm_request.response->offset + entry->base + (j * MIB(2)),
               PRESENT | RWALLOWED | WRITETHROUGH | PATBIT2MB);
      }
      hhdm_pages += page_amount;
      break;
    }
    default: {
      uint64_t disalign = entry->base % MIB(2);
      entry->base = align_down(entry->base, MIB(2));
      uint64_t page_amount =
          align_up(entry->length + disalign, MIB(2)) / MIB(2);
      for (uint64_t j = 0; j != page_amount; j++) {
        map2mb(take->root, entry->base + (j * MIB(2)),
               hhdm_request.response->offset + entry->base + (j * MIB(2)),
               PRESENT | RWALLOWED);
      }
      hhdm_pages += page_amount;
      break;
    }
    }
  }
  hhdm_pages = (hhdm_pages * MIB(2)) / 4096;
  return hhdm_pages;
}
void x86_64_init_pagemap(pagemap *take) {
  take->root =
      (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
}