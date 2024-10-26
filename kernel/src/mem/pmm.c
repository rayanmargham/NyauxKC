#include "pmm.h"
#include "limine.h"
#include "term/term.h"
#include "utils/basic.h"
#include <stdint.h>

typedef struct {
  struct pnode *next;
} __attribute__((packed)) pnode;
pnode head = {.next = NULL};
uint64_t get_free_pages();
spinlock_t pmmlock;
extern void *memset(void *s, int c, size_t n);
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
