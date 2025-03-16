#include "vmm.h"

#include <stdint.h>

#include "arch/arch.h"
#include "limine.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "utils/basic.h"

void kprintf_vmmregion(VMMRegion *region) {
  kprintf("\e[0;95mVMM Region {\r\n");
  kprintf(" base: 0x%lx\r\n", region->base);
  kprintf(" base + length: %lx\r\n", region->base + region->length);
  kprintf(" next: %p\r\n", (void *)region->next);
  kprintf("}\e[0;37m\r\n");
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
  VMMRegion *userstart = create_region(0, 0x1000);
  VMMRegion *userend = create_region(0x7fffffffa000, 0x1000);
  userstart->next = (struct VMMRegion *)userend;
  userend->next = NULL;
  map->userhead = (struct VMMRegion *)userstart;
  kprintf_vmmregion(hhdm);
  kprintf_vmmregion(kernel);
  res.type = OKAY;
  res.okay = true;
  return res;
}
void kprintf_all_vmm_regions() {
  VMMRegion *blah = (VMMRegion *)ker_map.head;
  while (blah != NULL) {
    kprintf_vmmregion(blah);
    blah = (VMMRegion *)blah->next;
  }
}
pagemap ker_map = {.head = NULL, .root = NULL};
void per_cpu_vmm_init() { arch_switch_pagemap(&ker_map); }
void vmm_init() {
  arch_init_pagemap(&ker_map);
  uint64_t hhdm_pages = arch_mapkernelhhdmandmemorymap(&ker_map);
  hhdm_pages = (hhdm_pages * MIB(2)) / 4096;
  kprintf("vmm(): HDDM Pages %lu\r\n", hhdm_pages);
  // panic("h");
  arch_switch_pagemap(&ker_map);
  kprintf("vmm(): Creating Regions...\r\n");
  result res = region_setup(&ker_map, hhdm_pages);
  unwrap_or_panic(res);
  kprintf("vmm(): Kernel is now in its own pagemap :)\r\n");
  kprintf("vmm(): Region is setup!\r\n");
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

void *kvmm_region_alloc(pagemap *map, uint64_t amount, uint64_t flags) {
  assert(ker_map.head != NULL);
  assert(ker_map.root != NULL);
  VMMRegion *cur = (VMMRegion *)map->head;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (prev == NULL) {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if ((cur->base - (prev->base + prev->length)) >= ARCH_CHECK_SPACE(amount)) {
      VMMRegion *new =
          create_region((prev->base + prev->length), align_up(amount, 4096));
      prev->next = (struct VMMRegion *)new;
      new->next = (struct VMMRegion *)cur;
      arch_map_vmm_region(&ker_map, new->base, new->length, false);
      return (void *)new->base;
    } else {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
  };
  kprintf("vmm(): No free Regions, Too Much Memory being used!!!\r\n");
  panic("vmm(): Sir madamm this should never occur");
  return NULL;
}
void *uvmm_region_alloc(pagemap *map, uint64_t amount, uint64_t flags) {
  assert(ker_map.userhead != NULL);
  assert(ker_map.root != NULL);
  VMMRegion *cur = (VMMRegion *)map->userhead;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (prev == NULL) {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if ((cur->base - (prev->base + prev->length)) >= ARCH_CHECK_SPACE(amount)) {
      VMMRegion *new =
          create_region((prev->base + prev->length), align_up(amount, 4096));
      prev->next = (struct VMMRegion *)new;
      new->next = (struct VMMRegion *)cur;
      arch_map_vmm_region(&ker_map, new->base, new->length, true);
      return (void *)new->base;
    } else {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
  };
  kprintf("vmm(): No free Regions, Too Much Memory being used!!!\r\n");
  panic("vmm(): Sir madamm this should never occur");
  return NULL;
}
void *uvmm_region_alloc_fixed(pagemap *map, uint64_t virt, size_t size,
                              bool force) {
  if (virt == 0) {
    return NULL;
  }
  VMMRegion *cur = (VMMRegion *)map->head;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (cur->base > (virt + size) && (prev->base + prev->length) < virt) {
      VMMRegion *new =
          create_region((prev->base + prev->length), align_up(size, 4096));
      prev->next = (struct VMMRegion *)new;
      new->next = (struct VMMRegion *)cur;
      arch_map_vmm_region(map, virt, size, true);
    }
    if (force && (prev->base + prev->length) < virt) {
      if (cur->base < size) {
        VMMRegion *tmp = (VMMRegion *)cur->next;
        arch_unmap_vmm_region(map, cur->base, cur->length);

        prev->next = (struct VMMRegion *)tmp;
        slabfree(cur);
      }
    }
    prev = cur;
    cur = (VMMRegion *)cur->next;
  }
  return NULL;
}
void kvmm_region_dealloc(pagemap *map, void *addr) {
  if (addr == NULL) {
    return;
  }
  VMMRegion *cur = (VMMRegion *)map->head;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (cur->base == (uint64_t)addr) {
      arch_unmap_vmm_region(&ker_map, cur->base, cur->length);
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
