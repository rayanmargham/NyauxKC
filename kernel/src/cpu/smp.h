#pragma once
#include <limine.h>
#include <stddef.h>
#include <term/term.h>
extern volatile struct limine_smp_request smp_request;
void init_smp();
