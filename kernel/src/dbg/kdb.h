#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
static const char *const cmds[] = {
    "exit", "mappings", "listvfs", "activethreads", "threads",
};
void rsh();
#ifdef __cplusplus
}
#endif