#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
void init_lapic();
void send_eoi();
#ifdef __cplusplus
}
#endif