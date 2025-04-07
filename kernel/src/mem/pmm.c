#include "pmm.h"

#include <stdint.h>

#include "limine.h"
#include "mem/vmm.h"
#include "term/term.h"
#include "utils/basic.h"

result init_kmalloc();
// OH MY GOD SO HARd
// THIS IS THE PMM
typedef struct {
  struct pnode *next;
} __attribute__((packed)) pnode;
pnode head = {.next = NULL};
uint64_t get_free_pages();
spinlock_t pmmlock;
extern void *memset(void *s, int c, size_t n);
// minnium 16 cache size
/* SLAB ALLOCATOR IGNORE*/
typedef struct {
  struct slab *next;
  struct pnode *freelist;
  uint64_t obj_ammount;
} __attribute__((packed)) slab;
typedef struct {
  uint64_t size;
  struct slab *slabs;
} cache;
cache caches[7];
/* SLAB ALLOCATOR IGNORE*/
result pmm_init() {
  spinlock_lock(&pmmlock);
  pnode *cur = &head;
  result ok = {.type = ERR, .err_msg = "pmm_init() failed"};
  kprintf("pmm_init(): entry count %lu\r\n",
          memmap_request.response->entry_count);
  kprintf("%s(): The HHDM is 0x%lx\r\n", __FUNCTION__,
          hhdm_request.response->offset);
  for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap_request.response->entries[i];
    switch (entry->type) {
    case LIMINE_MEMMAP_USABLE:
      for (uint64_t i = 0; i < entry->length; i += 4096) {
        pnode *hi = (pnode *)(entry->base + i + hhdm_request.response->offset);
        memset(hi, 0, 4096);
        cur->next = (struct pnode *)hi;
        cur = hi;
      }
      break;
    }
  }
  kprintf("pmm_init(): FreeList Created\r\n");
  kprintf("pmm_init(): Free Pages %ju\r\n", get_free_pages());
  spinlock_unlock(&pmmlock);
  result ress = init_kmalloc();
  unwrap_or_panic(ress);
  ok.type = OKAY;
  ok.okay = true;
  return ok;
}
// i get the free pages
uint64_t get_free_pages() {
  pnode *cur = &head;
  uint64_t page = 0;
  while (cur != NULL) {
    page += 1;
    cur = (pnode *)cur->next;
  };
  return page;
}
// Returns a page with the hddm offset added
// remove to get physical address
void *pmm_alloc() {
  if (head.next == NULL) {
    panic("no memory\r\n");
    return NULL;
  }
  pnode *him = (pnode *)head.next;
  if ((uint64_t)him < hhdm_request.response->offset) {
    sprintf("pmm(): addr is %p\r\n", him);
  }
  assert((uint64_t)him >= hhdm_request.response->offset);
  head.next = (struct pnode *)him->next;

  memset(him, 0, 4096); // just in case :)
  return (void *)him;
}
// I expect to get a page phys + hhdm offset added
// if i dont my assert will fail
void pmm_dealloc(void *he) {
  if (he == NULL) {
    return;
  }
  assert((uint64_t)he >= hhdm_request.response->offset);
  memset(he, 0, 4096); // just in case :)
  pnode *convert = (pnode *)he;
  convert->next = head.next;
  head.next = (struct pnode *)convert;
}

slab *init_slab(uint64_t size) {
  assert(size > sizeof(pnode));
  void *page = pmm_alloc();
  uint64_t obj_amount = (4096 - sizeof(slab)) / size;
  slab *hdr = (slab *)page;
  hdr->obj_ammount = obj_amount;
  pnode *first = (pnode *)((uint64_t)page + sizeof(slab));
  pnode *cur = first;
  first->next = NULL;
  hdr->freelist = (struct pnode *)first;
  for (uint64_t i = 1; i < obj_amount; i++) {
    pnode *new = (pnode *)((uint64_t)first + (i * size));
    new->next = NULL;
    cur->next = (struct pnode *)new;
    cur = new;
  };
  return hdr;
}
void init_cache(cache *mod, uint64_t size) {
  mod->size = size;
  mod->slabs = (struct slab *)init_slab(size);
}
result init_kmalloc() {
  result ok = {.type = ERR, .err_msg = "init_kmalloc(): failed"};
  init_cache(&caches[0], 16);
  init_cache(&caches[1], 32);
  init_cache(&caches[2], 64);
  init_cache(&caches[3], 128);
  init_cache(&caches[4], 256);
  init_cache(&caches[5], 512);
  init_cache(&caches[6], 1024);
  ok.type = OKAY;
  return ok;
}
void *slab_alloc(cache *mod) {
  if (mod->slabs == NULL || mod->size == 0)
    return NULL;

  slab *cur = (slab *)mod->slabs;
  assert(cur != NULL);

  while (cur->freelist == NULL) {
    if (cur->next == NULL) {
      cur->next = (struct slab *)init_slab(mod->size);
    }

    cur = (slab *)cur->next;
    // assert(cur != NULL);
  }

  void *ptr = cur->freelist;
  cur->freelist = ((pnode *)cur->freelist)->next;
  return ptr;
}
void *slaballocate(uint64_t amount) {
  uint64_t olda = amount;
  amount = next_pow2(amount);
  assert(olda <= amount);
  for (int i = 0; i != 7; i++) {
    cache *c = &caches[i];
    if (c->size < amount)
      continue;
    return slab_alloc(c);
  }
  return NULL;
}
uint64_t cache_getmemoryused(cache *mod) {
  if (mod->slabs == NULL || mod->size == 0) {
    return 0;
  }
  uint64_t bytes = 0;
  slab *cur = (slab *)mod->slabs;
  while (true) {
    if (cur->freelist == NULL) {
      bytes += sizeof(slab) + (cur->obj_ammount * mod->size);
      if (cur->next != NULL) {
        cur = (slab *)cur->next;
        continue;
      }
      break;
    }
    bytes += sizeof(slab);
    pnode *n = (pnode *)cur->freelist;
    uint64_t free_nodes = 0;
    while (true) {
      if (n->next == NULL) {
        free_nodes += 1;
        break;
      }
      free_nodes += 1;
      n = (pnode *)n->next;
    }
    bytes += ((cur->obj_ammount - free_nodes) * mod->size) +
             (free_nodes * sizeof(pnode));
    if (cur->next != NULL) {
      cur = (slab *)cur->next;
      continue;
    }
    break;
  };
  return bytes;
}

uint64_t total_memory() {
  uint64_t total_bytes = 0;
  for (int i = 0; i != 7; i++) {
    cache *c = &caches[i];
    total_bytes += cache_getmemoryused(c);
  }
  total_bytes += kvmm_region_bytesused();
  return total_bytes;
}
void slabfree(void *addr) {
  uint64_t real_addr = (uint64_t)addr;
  if (real_addr == 0) {
    return;
  }
  slab *guy = (slab *)(real_addr & ~0xFFF);
  memset(addr, 0, sizeof(pnode));
  pnode *node = (pnode *)addr;

  pnode *old = (pnode *)guy->freelist;

  node->next = (struct pnode *)old;
  guy->freelist = (struct pnode *)node;
  pnode *counter = (pnode *)guy->freelist;
  unsigned int howmanynodes = 0;
  while (counter != NULL) {
    howmanynodes += 1;
    counter = (pnode *)counter->next;
  }
  if (howmanynodes == guy->obj_ammount) {
    // kprintf("USELESS SLAB\r\n");
  }
}
