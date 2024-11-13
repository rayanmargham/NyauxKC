#pragma once
#include <stddef.h>
#include <term/term.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <utils/basic.h>
void init_hpet();
void stall_with_hpetclk(uint64_t ms);
