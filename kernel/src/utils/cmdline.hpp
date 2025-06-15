#pragma once
#include <stddef.h>
#ifdef __cplusplus
#include <frg/string.hpp>
#include <frg/allocation.hpp>
#endif
extern volatile struct limine_executable_cmdline_request limine_cmdline;

#ifdef __cplusplus
extern "C" void parse_cmdline();
#else
void parse_cmdline();
#endif
