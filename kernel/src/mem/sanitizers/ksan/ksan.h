#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <utils/basic.h>
#undef NYAUX_NO_KSAN
#define NYAUX_NO_KSAN __attribute__((no_sanitize("address")))
#ifdef __cplusplus
}
#endif
