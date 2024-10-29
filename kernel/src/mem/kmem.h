#include "pmm.h"
#include "vmm.h"
#include <limine.h>
#include <stdint.h>
void kfree(void *addr, uint64_t size);
void *kmalloc(uint64_t amount);
