#pragma once
#include <limine.h>
#include <stdint.h>
#include <utils/basic.h>
result pmm_init();
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
void *slaballocate(uint64_t amount);
void slabfree(void *addr);

void *pmm_alloc();
void pmm_dealloc(void *he);
uint64_t total_memory();
void free_unused_slabcaches();
