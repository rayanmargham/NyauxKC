#pragma once
#include "../instructions/instructions.h"
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
void fpu_init();
extern void (*fpu_save)(void *area);
extern void (*fpu_store)(void *area);