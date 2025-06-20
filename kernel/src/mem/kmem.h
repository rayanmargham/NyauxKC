#pragma once

#include "pmm.h"
#include "vmm.h"
#include <limine.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kfree(void *addr, uint64_t size);
void *kmalloc(uint64_t amount);
extern void *memset(void *s, int c, size_t n);
#ifdef __cplusplus
}
#endif