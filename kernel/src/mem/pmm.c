#include "pmm.h"

#include <stdint.h>

#include "arch/arch.h"
#include "limine.h"
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"

result init_kmalloc();
// OH MY GOD SO HARd
// THIS IS THE PMM
typedef struct {
  void *ptr;
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
pnode **pagedatabase;
/* SLAB ALLOCATOR IGNORE*/

result pmm_init() {
  spinlock_lock(&pmmlock);
  result ok = {.type = ERR, .err_msg = "pmm_init() failed"};
  kprintf("pmm_init(): entry count %lu\r\n",
          memmap_request.response->entry_count);
  kprintf("%s(): The HHDM is 0x%lx\r\n", __FUNCTION__,
          hhdm_request.response->offset);
  size_t num_of_pages = 0;
  struct limine_memmap_response *response = memmap_request.response;
  uint64_t max_phys_addr = 0;
  for (size_t i = 0; i < response->entry_count; i++) {
    struct limine_memmap_entry *entry = response->entries[i];
    switch(entry->type) {
      case LIMINE_MEMMAP_USABLE:
        if (entry->base == 0x0) {
          continue;
        }
        num_of_pages += entry->length / 4096;
        max_phys_addr = entry->base + entry->length;
      default:
        break;
    }
  }
  kprintf_log(TRACE, "num of pages: %lu\r\n", num_of_pages);
  bool done_freelist_allocation = false;
  bool done_db_alloc = false;
  for (size_t i = 0; i < response->entry_count; i++) {
    struct limine_memmap_entry *entry = response->entries[i];
    switch(entry->type) {
      case LIMINE_MEMMAP_USABLE:
        if (entry->base == 0x0) {
          continue;
        }
        if (entry->length >= sizeof(pnode) * align_up(max_phys_addr, 4096) / 4096 && !done_freelist_allocation) {
          pnode *prev = NULL;
          for (size_t j = 0; j < sizeof(pnode) * (max_phys_addr/4096); j += sizeof(pnode)) {
             pnode *new_pnode = (pnode*)(entry->base + j + hhdm_request.response->offset);
              if (prev == NULL) {
                head.next = (struct pnode*)new_pnode;
              } else {
                prev->next = (struct pnode*)new_pnode;
              }
              prev = new_pnode;

          }
          prev->next = NULL;
          entry->base += align_up(sizeof(pnode) * max_phys_addr / 4096, 4096);
          entry->length -= align_up(sizeof(pnode) * max_phys_addr / 4096, 4096);
          done_freelist_allocation = true;
        }
        if (entry->length >= sizeof(pnode*) * align_up(max_phys_addr, 4096) / 4096) {
          for (size_t j = 0; j < sizeof(pnode*) * (max_phys_addr/4096); j += sizeof(pnode*)) {
            pnode **new_ptr = (pnode**)(entry->base + j + hhdm_request.response->offset);
            if (j == 0) {
              pagedatabase = new_ptr;
            }
            *new_ptr = NULL;
          }
          entry->base += align_up(sizeof(pnode*) * max_phys_addr/4096, 4096);
          entry->length -= align_up(sizeof(pnode*) *max_phys_addr/4096, 4096);
          done_db_alloc = true;
          if (done_db_alloc && done_freelist_allocation) {
            goto finish;
          }
        }
        break;
      default:
        break;
    }
  }
  finish:
  pnode *start = (pnode*)head.next;
  for (size_t i = 0; i < response->entry_count; i++) {
    struct limine_memmap_entry *entry = response->entries[i];
    switch(entry->type) {
      case LIMINE_MEMMAP_USABLE:
        if (entry->base == 0x0) {
          continue;
        }

        for (size_t j = 0; j < entry->length; j += 0x1000) {
          start->ptr = (void*)((size_t)entry->base + j);
          if (start->next == 0x0) {
            goto done;
          }
          start = (pnode*)start->next;
        }
      default:
        break;
    }
  }
  start->next = NULL;
  done:
  kprintf_log(STATUSOK, "pmm_init(): FreeList Created\r\n");
  kprintf_log(LOG, "pmm_init(): Free Pages %ju\r\n", get_free_pages());
  spinlock_unlock(&pmmlock);
  result ress = init_kmalloc();
  unwrap_or_panic(ress);
  ok.type = OKAY;
  ok.okay = true;
  return ok;
}
// i get the free pages
uint64_t get_free_pages() {
  pnode *cur = (pnode*)head.next;
  uint64_t page = 0;
  while (cur != NULL) {
    page += 1;
    cur = (pnode *)cur->next;
  };
  return page;
}
// Returns a page with the hddm offset added
// remove to get physical address
size_t pages_alloced = 0;
void *pmm_alloc() {
  if (head.next == NULL) {
    panic("no memory");
    return NULL;
  }
  // panic("no");
  pnode *him = (pnode *)head.next;
  head.next = (struct pnode *)him->next;
  memset((void*)(((uint64_t)him->ptr) + hhdm_request.response->offset), 0, 4096); // just in case :)
  pagedatabase[(size_t)him->ptr >> 12] = him;
  pages_alloced += 1;
  //sprintf("pages alloc: %lu, free pages: %lu\r\n", pages_alloced, get_free_pages());


  return (void*)(((uint64_t)him->ptr) + hhdm_request.response->offset);
}

// I expect to get a page phys + hhdm offset added
// if i dont my assert will fail
void pmm_dealloc(void *he) {
  if (he == NULL) {
    return;
  }
  memset((void*)((size_t)he), 0, 4096); // just in case :)
  pnode *convert = (pnode *)pagedatabase[((size_t)he - hhdm_request.response->offset) >> 12];
  convert->next = head.next;
  head.next = (struct pnode *)convert;
  pagedatabase[((size_t)he - hhdm_request.response->offset) >> 12] = NULL;
}

slab *init_slab(uint64_t size) {
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
    // implementing this involes making the slab list doubly linked
    // which is not the focus of Nyaux rn.
    // TODO: do this memory optimization later.
    // kprintf("USELESS SLAB\r\n");
  }
}
