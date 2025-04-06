#include "pt.h"

#include <stdint.h>

#include "arch/x86_64/instructions/instructions.h"
#include "limine.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "term/term.h"
#include "utils/basic.h"

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
      page_table[idx] = (uint64_t)guy | PRESENT | RWALLOWED | USERMODE;
      pt = guy;
    } else if (page_table[idx] & PAGE2MB) {
      uint64_t *guy = (uint64_t *)((uint64_t)pmm_alloc());
      uint64_t old_phys = page_table[idx] & 0x000ffffffffff000;
      uint64_t old_flags = page_table[idx] & ~0x000ffffffffff000;
      for (int j = 0; j < 512; j++) {
        guy[j] = (old_phys + j * PAGESIZE) | (old_flags & ~PAGE2MB);
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

  switch_to_pagemap((uint64_t)(take->root));
}
uint64_t unmap(uint64_t *pt, uint64_t virt) {
  if (virt < (hhdm_request.response->offset + (hhdm_pages * 0x1000)) &&
      (virt > (hhdm_request.response->offset))) {
    // make sure we dont accidentely unmap the hhdm
    sprintf("ignoring virt 0x%lx\r\n", virt);
    return 0;
  }
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
  uint64_t length_in_4kib = length_in_bytes / PAGESIZE;
  for (uint64_t i = 0; i != length_in_4kib; i++) {
    void *page =
        (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
    map(take->root, (uint64_t)page, base + (i * PAGESIZE), PRESENT | RWALLOWED);
  }
}
void x86_64_map_vmm_region_user(pagemap *take, uint64_t base,
                                uint64_t length_in_bytes) {
  uint64_t length_in_4kib = length_in_bytes / PAGESIZE;
  for (uint64_t i = 0; i != length_in_4kib; i++) {
    void *page =
        (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
    map(take->root, (uint64_t)page, base + (i * PAGESIZE),
        PRESENT | RWALLOWED | USERMODE);
  }
}
uint64_t x86_64_get_phys(pagemap *take, uint64_t virt) {
  uint64_t *f = find_pte(take->root, virt);
  if (f != NULL) {
    return pte_to_phys(*f);
  }
  return 0;
}
void x86_64_unmap_vmm_region(pagemap *take, uint64_t base,
                             uint64_t length_in_bytes) {
  uint64_t amount_to_allocateinpages = length_in_bytes / PAGESIZE;
  for (uint64_t i = 0; i != amount_to_allocateinpages; i++) {
    uint64_t phys = unmap(take->root, base + (i * PAGESIZE));
    pmm_dealloc((void *)(phys + hhdm_request.response->offset));
  }
}
static void destroy_page_table(uint64_t *table, int level) {
  // we are already in a differnt page table so this should be fine
  uint64_t *vtable =
      (uint64_t *)((uint64_t)table + hhdm_request.response->offset);
  if (level != 0) {
    for (int i = 0; i < 512; i++) {
      if (vtable[i] & PRESENT) {
        if (level < 3 && !(vtable[i] & PAGE2MB)) {
          uint64_t *next_table =
              (uint64_t *)(((uint64_t)(pte_to_phys(vtable[i]))));
          destroy_page_table(next_table, level + 1);
          pmm_dealloc(vtable);
        }
      }
    }
  } else {
    for (int i = 0; i < 256; i++) {
      if (vtable[i] & PRESENT) {
        if (level < 3 && !(vtable[i] & PAGE2MB)) {
          uint64_t *next_table =
              (uint64_t *)(((uint64_t)(pte_to_phys(vtable[i]))));
          destroy_page_table(next_table, level + 1);
          pmm_dealloc(vtable);
        }
      }
    }
  }
  pmm_dealloc(
      (void *)(pte_to_phys((uint64_t)table) + hhdm_request.response->offset));
}
void x86_64_destroy_pagemap(pagemap *take) {
  if (take && take->root) {
    destroy_page_table(take->root, 0);
    sprintf("okay\r\n");
    sprintf("take root is %p\r\n", take->root);
    pmm_dealloc(
        (void *)(((uint64_t)take->root) + hhdm_request.response->offset));
    take->root = NULL;
  }
}
bool is2mibaligned(uint64_t addr, uint64_t length) {
  if (is_aligned(addr, MIB(2)) && length >= MIB(2)) {
    return true;
  } else {
    return false;
  }
}
uint64_t x86_64_map_kernelhhdmandmemorymap(pagemap *take) {
  uint64_t kernel_size = align_up((uint64_t)THE_REAL, PAGESIZE);
  for (uint64_t i = 0; i != kernel_size; i += PAGESIZE) {
    map(take->root, kernel_address.response->physical_base + i,
        kernel_address.response->virtual_base + i, PRESENT | RWALLOWED);
  }
  kprintf("vmm(x86_64)(): mapped 0x%lx to 0x%lx : from phys 0x%lx to 0x%lx\r\n",
          kernel_address.response->virtual_base,
          kernel_address.response->virtual_base + kernel_size,
          kernel_address.response->physical_base,
          kernel_address.response->physical_base + kernel_size);
  kprintf("vmm(x86_64)(): Kernel Mapped!\r\n");
  uint64_t hhdm_pages = 0;
  for (uint64_t i = 0; i < 0x100000000; i += MIB(2)) {
    assert(i % MIB(2) == 0);
    assert(is2mibaligned(0x0, MIB(2)) == true);
    map2mb(take->root, i, hhdm_request.response->offset + i,
           PRESENT | RWALLOWED);
    hhdm_pages += 1;
  }
  kprintf("vmm(x86_64)(): (4 GiB region) mapping 0x%lx to %lx : phys 0x%lx to "
          "0x%lx",
          hhdm_request.response->offset,
          hhdm_request.response->offset + 0x100000000, (uint64_t)0,
          0x100000000);
  kprintf("vmm(x86_64)(): Above 4Gib Mapped! Mapping Memory Map!\r\n");
  for (uint64_t i = 0; i != memmap_request.response->entry_count; i += 1) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    switch (entry->type) {
    case LIMINE_MEMMAP_FRAMEBUFFER: {
      uint64_t addr = entry->base;
      uint64_t length = entry->length;
      kprintf("vmm()(x86_64): framebuffer located at %p, length is %lu\r\r\n",
              (uint64_t *)addr, length);
      while (length >= PAGESIZE) {
        if (is2mibaligned(addr, MIB(2)) && length >= MIB(2)) {
          map2mb(take->root, entry->base + length,
                 hhdm_request.response->offset + entry->base + length,
                 PRESENT | WRITETHROUGH | RWALLOWED | PATBIT2MB);
          addr += MIB(2);
          length -= MIB(2);
        } else {
          map(take->root, entry->base + length,
              hhdm_request.response->offset + entry->base + length,
              PRESENT | RWALLOWED);
          addr += PAGESIZE;
          length -= PAGESIZE;
        }
      }
    } break;

    case LIMINE_MEMMAP_BAD_MEMORY: {
      break;
    }
    case LIMINE_MEMMAP_RESERVED: {
      break;
    }
    default: {
      uint64_t addr = entry->base;
      uint64_t length = entry->length;
      while (length >= PAGESIZE) {
        if (is2mibaligned(addr, MIB(2)) && length >= MIB(2)) {
          map2mb(take->root, entry->base + length,
                 hhdm_request.response->offset + entry->base + length,
                 PRESENT | RWALLOWED);
          hhdm_pages += MIB(2);
          addr += MIB(2);
          length -= MIB(2);
        } else {
          map(take->root, entry->base + length,
              hhdm_request.response->offset + entry->base + length,
              PRESENT | RWALLOWED);
          hhdm_pages += 1;
          addr += PAGESIZE;
          length -= PAGESIZE;
        }
      }
      break;
    }
    }
  }
  hhdm_pages = hhdm_pages / PAGESIZE;
  return hhdm_pages;
}
void x86_64_init_pagemap(pagemap *take) {
  take->root =
      (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
}
