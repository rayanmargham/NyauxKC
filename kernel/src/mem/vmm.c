#include "vmm.h"

#include <stddef.h>
#include <stdint.h>

#include "arch/arch.h"
#include "arch/x86_64/page_tables/pt.h"
#include "fs/devfs/devfs.h"
#include "limine.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "utils/basic.h"

void kprintf_vmmregion(VMMRegion *region) {
  sprintf("VMM Region \e[0;95m{\r\n");
  sprintf(" base: 0x%lx\r\n", region->base);
  sprintf(" base + length: %lx\r\n", region->base + region->length);
  sprintf(" next: %p\r\n", (void *)region->next);
  sprintf("}\e[0;37m\r\n");
}
VMMRegion *create_region(uint64_t base, uint64_t length) {
  VMMRegion *bro = slaballocate(sizeof(VMMRegion));
  bro->base = base;
  bro->length = length;
  bro->nocopy = false;
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
  kernel->nocopy = true;
  hhdm->nocopy = true;
  map->head = (struct VMMRegion *)hhdm;
  VMMRegion *userstart = create_region(0, 0x1000);
  VMMRegion *userend = create_region(0x7fffffffa000, 0x1000);
  userstart->next = (struct VMMRegion *)userend;
  userend->next = NULL;
  userend->nocopy = true;
  userstart->nocopy = true;
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
void kprintf_all_uvmm_regions() {
  VMMRegion *blah = (VMMRegion *)ker_map.userhead;
  while (blah != NULL) {
    kprintf_vmmregion(blah);
    blah = (VMMRegion *)blah->next;
  }
}
uint64_t hhdm_pages;
pagemap ker_map = {.head = NULL, .root = NULL};
void per_cpu_vmm_init() { arch_switch_pagemap(&ker_map); }
void vmm_init() {
  arch_init_pagemap(&ker_map);
  hhdm_pages = arch_mapkernelhhdmandmemorymap(&ker_map);
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
  assert(map->userhead != NULL);
  assert(map->root != NULL);
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
      arch_map_vmm_region(map, new->base, new->length, true);
      sprintf("uvm_region_alloc(): returning %p\r\n", (void *)new->base);
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
  VMMRegion *cur = (VMMRegion *)map->userhead;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (prev == NULL) {
      prev = cur;
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if (cur->base >= (virt + size) && (prev->base + prev->length) <= virt) {
      VMMRegion *new = create_region(virt, align_up(size, 4096));
      prev->next = (struct VMMRegion *)new;
      new->next = (struct VMMRegion *)cur;
      arch_map_vmm_region(map, new->base, new->length, true);
      return (void *)virt;
    }
    if (force && (prev->base + prev->length) <= virt) {
      if (cur->base <= (virt + size)) {
        VMMRegion *tmp = (VMMRegion *)cur->next;
        arch_unmap_vmm_region(map, cur->base, cur->length);

        slabfree(cur);
        VMMRegion *new = create_region(virt, align_up(size, 4096));
        prev->next = (struct VMMRegion *)new;
        new->next = (struct VMMRegion *)tmp;
        arch_map_vmm_region(map, virt, size, true);
        return (void *)virt;
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
      arch_unmap_vmm_region(map, cur->base, cur->length);
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
void uvmm_region_dealloc(pagemap *map, void *addr) {
  if (addr == NULL) {
    return;
  }
  VMMRegion *cur = (VMMRegion *)map->userhead;
  VMMRegion *prev = NULL;
  while (cur != NULL) {
    if (cur->base == (uint64_t)addr) {
      arch_unmap_vmm_region(map, cur->base, cur->length);
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
pagemap *new_pagemap() {
  pagemap *we = kmalloc(sizeof(pagemap));
  arch_completeinit_pagemap(we);
  result res = region_setup(we, hhdm_pages);
  unwrap_or_panic(res);
  return we;
}
void duplicate_pagemap(pagemap *maptoduplicatefrom, pagemap *to) {
  assert(to != NULL);
  for (int i = 511; i >= 256; i--) {
    ((uint64_t *)((uint64_t)to->root + hhdm_request.response->offset))[i] =
        ((uint64_t *)((uint64_t)maptoduplicatefrom->root +
                      hhdm_request.response->offset))[i];
  }

  VMMRegion *inital = (VMMRegion *)maptoduplicatefrom->head;
  while (inital != NULL) {
    // Copy VMM Regions
    if (inital->nocopy) {
      inital = (VMMRegion *)inital->next;
      continue;
    }

    if (!arch_get_phys(to, inital->base)) {
      arch_map_vmm_region(to, inital->base, inital->length, false);
      for (size_t i = 0; i < inital->length; i += 0x1000) {
        memcpy((void *)(arch_get_phys(to, inital->base + i) +
                        hhdm_request.response->offset),
               (void *)inital->base + i, 0x1000);
      }
    }
    VMMRegion *e = create_region(inital->base, inital->length);
    e->next = to->head;
    to->head = (struct VMMRegion *)e;
    inital = (VMMRegion *)inital->next;
  }

  VMMRegion *user = (VMMRegion *)maptoduplicatefrom->userhead;
  while (user != NULL) {
    // Copy VMM Regions
    if (user->nocopy) {
      user = (VMMRegion *)user->next;
      continue;
    }
    if (!arch_get_phys(to, user->base)) {

      arch_map_vmm_region(to, user->base, user->length, true);
      for (size_t i = 0; i < user->length; i += 0x1000) {
        memcpy((void *)(arch_get_phys(to, user->base + i) +
                        hhdm_request.response->offset),
               (void *)user->base + i, 0x1000);
      }
    }
    VMMRegion *e = create_region(user->base, user->length);
    e->next = to->userhead;
    to->userhead = (struct VMMRegion *)e;
    user = (VMMRegion *)user->next;
  }
}
void deallocate_all_user_regions(pagemap *target) {
  VMMRegion *cur = (VMMRegion *)target->userhead;
  while (cur != NULL) {
    if (cur->nocopy) {
      slabfree(cur);
      cur = (VMMRegion *)cur->next;
      continue;
    }
    arch_unmap_vmm_region(target, cur->base, cur->length);
    slabfree(cur);

    cur = (VMMRegion *)cur->next;
  }
  assert(cur == NULL);
  VMMRegion *userstart = create_region(0, 0x1000);
  VMMRegion *userend = create_region(0x7fffffffa000, 0x1000);
  userstart->next = (struct VMMRegion *)userend;
  userend->next = NULL;
  userend->nocopy = true;
  userstart->nocopy = true;
  target->userhead = (struct VMMRegion *)userstart;
}
void deallocate_all_kernel_regions_and_user(pagemap *target) {
  VMMRegion *cur = (VMMRegion *)target->head;
  while (cur != NULL) {
    if (cur->nocopy) {
      slabfree(cur);
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if (!arch_get_phys(target, cur->base)) {

      arch_unmap_vmm_region(target, cur->base, cur->length);
    }
    slabfree(cur);

    cur = (VMMRegion *)cur->next;
  }
  assert(cur == NULL);
  cur = (VMMRegion *)target->userhead;
  while (cur != NULL) {
    if (cur->nocopy) {
      slabfree(cur);
      cur = (VMMRegion *)cur->next;
      continue;
    }
    if (!arch_get_phys(target, cur->base)) {

      arch_unmap_vmm_region(target, cur->base, cur->length);
    }
    slabfree(cur);

    cur = (VMMRegion *)cur->next;
  }
  assert(cur == NULL);
}
void free_pagemap(pagemap *take) {
  deallocate_all_kernel_regions_and_user(take);
  arch_destroy_pagemap(take);
  sprintf("done?\r\n");
  kfree(take, sizeof(pagemap));
}