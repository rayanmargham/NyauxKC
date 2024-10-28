#pragma once
#include <limine.h>
#include <stdint.h>
#include <utils/basic.h>
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern char THE_REAL[];
extern volatile struct limine_kernel_address_request kernel_address;
void vmm_init();