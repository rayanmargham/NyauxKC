#pragma once
#include <limine.h>
#include <stdint.h>
#include <utils/basic.h>
result pmm_init();
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
void *kmalloc(uint64_t amount);
void free(void *addr);