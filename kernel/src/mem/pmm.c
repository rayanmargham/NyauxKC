#include "pmm.h"
#include "limine.h"
#include "term/term.h"
#include "utils/basic.h"

#include <stdint.h>
result init_kmalloc();
typedef struct {
  struct pnode *next;
} __attribute__((packed)) pnode;
pnode head = {.next = NULL};
uint64_t get_free_pages();
spinlock_t pmmlock;
extern void *memset(void *s, int c, size_t n);
// minnium 16 cache size
typedef struct {
  struct slab *next;
  struct pnode *freelist;
} __attribute__((packed)) slab;
typedef struct {
  uint64_t size;
  struct slab *slabs;
} cache;
cache caches[7];
result pmm_init() {
  spinlock_lock(&pmmlock);
  pnode *cur = &head;
  result ok = {.type = ERR, .err_msg = "pmm_init() failed"};
  kprintf("pmm_init(): entry count %d\n", memmap_request.response->entry_count);
  kprintf("%s(): The HHDM is 0x%lx\n", __FUNCTION__,
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
  kprintf("FreeList Created\n");
  kprintf("Free Pages %ju\n", get_free_pages());
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
    return NULL;
  }
  pnode *him = (pnode *)head.next;
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
  void *page = pmm_alloc();
  uint64_t obj_amount = (4096 - sizeof(slab)) / size;
  slab *hdr = (slab *)page;
  pnode *first = (pnode *)((uint64_t)page + sizeof(slab));
  pnode *cur = first;
  hdr->freelist = (struct pnode *)first;
  for (uint64_t i = 1; i < obj_amount; i++) {
    pnode *new = (pnode *)((uint64_t)first + (i * size));
    new->next = (struct pnode *)cur;
    cur = new;
  };
  return hdr;
}
void init_cache(cache *mod, uint64_t size) {
  mod->size = size;
  mod->slabs = (struct slab *)init_slab(size);
}
result init_kmalloc() {
  spinlock_lock(&pmmlock);
  result ok = {.type = ERR, .err_msg = "init_kmalloc() failed"};
  init_cache(&caches[0], 16);
  init_cache(&caches[0], 32);
  init_cache(&caches[0], 64);
  init_cache(&caches[0], 128);
  init_cache(&caches[0], 256);
  init_cache(&caches[0], 512);
  init_cache(&caches[0], 1024);
  ok.type = OKAY;
  spinlock_unlock(&pmmlock);
  return ok;
}
void *slab_alloc(cache *mod) {
  spinlock_lock(&pmmlock);
  if (mod->slabs == NULL || mod->size == 0) {
    return NULL;
  }
  slab *cur = (slab *)mod->slabs;
  while (true) {
    if (cur->freelist == NULL) {
      if (cur->next != NULL) {
        cur = (slab *)cur->next;
        continue;
      }
      break;
    }
    void *guy = cur->freelist;
    cur->freelist = ((pnode *)cur->freelist)->next;
    spinlock_unlock(&pmmlock);
    return guy;
  };
  cur->next = (struct slab *)init_slab(mod->size);
  cur = (slab *)cur->next;
  void *guy = cur->freelist;
  cur->freelist = ((pnode *)cur->freelist)->next;
  spinlock_unlock(&pmmlock);
  return guy;
}
void *slaballocate(uint64_t amount) {
  amount = next_pow2(amount);
  for (int i = 0; i != 7; i++) {
    cache *c = &caches[i];
    assert(c->size != 0);
    return slab_alloc(c);
  }
  return NULL;
}
void slabfree(void *addr) {
  spinlock_lock(&pmmlock);
  uint64_t real_addr = (uint64_t)addr;
  if (real_addr == 0) {
    spinlock_unlock(&pmmlock);
    return;
  }
  slab *guy = (slab *)(real_addr & ~0xFFF);
  memset(addr, 0, sizeof(pnode));
  pnode *node = (pnode *)addr;
  pnode *old = (pnode *)guy->freelist;
  node->next = (struct pnode *)old;
  guy->freelist = (struct pnode *)node;
  spinlock_unlock(&pmmlock);
}