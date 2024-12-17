#pragma once
#include <limine.h>
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>
extern volatile struct limine_mp_request smp_request;
void init_smp();
