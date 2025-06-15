#pragma once
#include <mem/kmem.h>
#ifdef __cplusplus
extern "C" {
#endif
void frg_log(const char *cstring);
void frg_panic(const char *cstring);
#ifdef __cplusplus
}
#endif
