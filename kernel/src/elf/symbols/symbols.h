#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <mem/kmem.h>
#include <stddef.h>
#include <stdint.h>
#include <utils/basic.h>

#include "../elf.h"
#include "term/term.h"
void get_symbols();
typedef struct {
  char *function_name;
  uint64_t function_address;
  size_t function_size;
} nyauxsymbol;
typedef struct {
  nyauxsymbol *array;
  size_t size;
} nyauxsymbolresource;
extern nyauxsymbolresource *symbolarray;
extern void *memset(void *s, int c, size_t n);
nyauxsymbol find_from_rip(uint64_t rip);
#ifdef __cplusplus
}
#endif