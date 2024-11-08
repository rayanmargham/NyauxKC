#include "vmm.h"
#include "limine.h"
#include "mem/pmm.h"
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
        panic("find_pte error: LargePageSize bit set in PML4 entry");
      else if (i == 1)
        kprintf("find_pte warning: Found 1GiB page PDPT entry");

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
void switch_cr3(uint64_t pml4) { switch_to_pagemap(pml4); }
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

typedef struct {
  uint64_t *pml4;
  struct VMMRegion *head;
} pagemap;
typedef struct {
  uint64_t base;
  uint64_t length;
  struct VMMRegion *next;
} VMMRegion;

pagemap ker_map;
void kprintf_vmmregion(VMMRegion *region) {
  kprintf("\e[0;95mVMM Region {\n");
  kprintf(" base: 0x%lx\n", region->base);
  kprintf(" length: %u\n", region->length);
  kprintf(" next: %p\n", region->next);
  kprintf("}\e[0;37m\n");
}
VMMRegion *create_region(uint64_t base, uint64_t length) {
  VMMRegion *bro = slaballocate(sizeof(VMMRegion));
  bro->base = base;
  bro->length = length;
  return bro;
}
result region_setup(pagemap *map, uint64_t hddm_in_pages) {
  result res = {.err_msg = "Failed to Create Regions", .type = OKAY};
  uint64_t kernel_size = align_up((uint64_t)THE_REAL, 4096);
  VMMRegion *kernel =
      create_region(kernel_address.response->virtual_base, kernel_size);
  VMMRegion *hhdm =
      create_region(hhdm_request.response->offset, hddm_in_pages * 4096);
  hhdm->next = (struct VMMRegion *)kernel;
  kernel->next = NULL;
  map->head = (struct VMMRegion *)hhdm;
  kprintf_vmmregion(hhdm);
  kprintf_vmmregion(kernel);
  res.type = OKAY;
  res.okay = true;
  return res;
}

void vmm_init() {
  ker_map.pml4 =
      (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
  uint64_t kernel_size = align_up((uint64_t)THE_REAL, 4096);
  for (uint64_t i = 0; i != kernel_size; i += 4096) {
    map(ker_map.pml4, kernel_address.response->physical_base + i,
        kernel_address.response->virtual_base + i, PRESENT | RWALLOWED);
  }
  kprintf("Kernel Mapped!\n");
  uint64_t hhdm_pages = 0;
  for (uint64_t i = 0; i < 0x100000000; i += 2097152) {
    assert(i % 2097152 == 0);
    map2mb(ker_map.pml4, i, hhdm_request.response->offset + i,
           PRESENT | RWALLOWED);
    hhdm_pages += 1;
  }
  kprintf("Above 4Gib Mapped! Mapping Memory Map!\n");
  for (uint64_t i = 0; i != memmap_request.response->entry_count; i += 1) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    switch (entry->type) {
    case LIMINE_MEMMAP_FRAMEBUFFER: {
      uint64_t disalign = entry->base % 2097152;
      entry->base = align_down(entry->base, 2097152);
      uint64_t page_amount =
          align_up(entry->length - disalign, 2097152) / 2097152;
      for (uint64_t j = 0; j != page_amount; j++) {
        map2mb(ker_map.pml4, entry->base + (j * 2097152),
               hhdm_request.response->offset + entry->base + (j * 2097152),
               PRESENT | RWALLOWED | WRITETHROUGH | PATBIT2MB);
      }
      hhdm_pages += page_amount;
      break;
    }
    default: {
      uint64_t disalign = entry->base % 2097152;
      entry->base = align_down(entry->base, 2097152);
      uint64_t page_amount =
          align_up(entry->length - disalign, 2097152) / 2097152;
      for (uint64_t j = 0; j != page_amount; j++) {
        map2mb(ker_map.pml4, entry->base + (j * 2097152),
               hhdm_request.response->offset + entry->base + (j * 2097152),
               PRESENT | RWALLOWED);
      }
      hhdm_pages += page_amount;
      break;
    }
    }
  }
  hhdm_pages = (hhdm_pages * 2097152) / 4096;
  kprintf("HDDM Pages %d\n", hhdm_pages);
  // panic("h");
  switch_cr3((uint64_t)(ker_map.pml4));
  result res = region_setup(&ker_map, hhdm_pages);
  unwrap_or_panic(res);
  kprintf("Kernel is now in its own pagemap :)\n");
  kprintf("Region is setup!\n");
}
uint64_t kvmm_region_bytesused() {
  VMMRegion *cur = (VMMRegion *)ker_map.head;
  uint64_t bytes = 0;
  while (true) {
    if (cur->base == 0xffffffff80000000 || cur->base == 0xffff800000000000) {
      if (cur->next == NULL) {
        break;
      } else {
        cur = (VMMRegion *)cur->next;
        continue;
      }
    }
    bytes += cur->length;
    if (cur->next == NULL) {
      break;
    } else {
      cur = (VMMRegion *)cur->next;
      continue;
    }
  }
  return bytes;
}
void *kvmm_region_alloc(uint64_t amount, uint64_t flags) {
  assert(ker_map.head != NULL);
  assert(ker_map.pml4 != NULL);
  VMMRegion *cur = (VMMRegion *)ker_map.head;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (prev == NULL) {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if ((cur->base - (prev->base + prev->length)) >=
        align_up(amount, 4096) + 0x1000) {
      VMMRegion *new =
          create_region((prev->base + prev->length), align_up(amount, 4096));
      prev->next = (struct VMMRegion *)new;
      new->next = (struct VMMRegion *)cur;
      uint64_t amount_to_allocateinpages = new->length / 4096;
      for (uint64_t i = 0; i != amount_to_allocateinpages; i++) {
        void *page =
            (uint64_t *)((uint64_t)pmm_alloc() - hhdm_request.response->offset);
        map(ker_map.pml4, (uint64_t)page, new->base + (i * 4096), flags);
      }
      memset((void *)new->base, 0, new->length);
      return (void *)new->base;
    } else {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
  };
  kprintf("No free Regions, Too Much Memory being used!!!\n");
  panic("Sir madamm this should never occur");
  return NULL;
}
void kvmm_region_dealloc(void *addr) {
  if (addr == NULL) {
    return;
  }
  VMMRegion *cur = (VMMRegion *)ker_map.head;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (cur->base == (uint64_t)addr) {
      memset((void *)cur->base, 0, cur->length);
      uint64_t amount_to_allocateinpages = cur->length / 4096;
      for (uint64_t i = 0; i != amount_to_allocateinpages; i++) {
        uint64_t phys = unmap(ker_map.pml4, cur->base + (i * 4096));
        pmm_dealloc((void *)(phys + hhdm_request.response->offset));
      }
      if (prev != NULL)
        prev->next = cur->next;
      slabfree(cur);

      return;
    } else {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
  }
}
