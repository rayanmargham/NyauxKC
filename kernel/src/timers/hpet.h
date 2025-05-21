#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <term/term.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <utils/basic.h>
void init_hpet();
void stall_with_hpetclk(uint64_t ms);
uint64_t read_hpet_counter();
void stall_with_hpetclkmicro(uint64_t usec);
#ifdef __cplusplus
}
#endif
