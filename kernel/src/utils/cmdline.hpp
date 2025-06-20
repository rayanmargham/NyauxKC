#pragma once
#include <stddef.h>
#include <utils/basic.h>
#include <mem/kmem.h>
#ifdef __cplusplus
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <cppglue/glue.hpp>
#include <frg/allocation.hpp>
#endif
extern volatile struct limine_executable_cmdline_request limine_cmdline;

#ifdef __cplusplus
extern "C" void parse_cmdline();
// @return returns cmdline object
extern "C" void *look_for_option(const char *key);
#else
void parse_cmdline();
// @return returns cmdline object
void *look_for_option(const char *key);
#endif
