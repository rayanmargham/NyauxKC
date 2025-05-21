#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <utils/basic.h>
#undef NYAUX_NO_UBSAN
#define NYAUX_NO_UBSAN __attribute__((no_sanitize("undefined")))
#ifdef __cplusplus
}
#endif
