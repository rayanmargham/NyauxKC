#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "../instructions/instructions.h"
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
void fpu_init();
extern void (*fpu_save)(void *area);
extern void (*fpu_store)(void *area);
uint64_t get_fpu_storage_size();
#ifdef __cplusplus
}
#endif