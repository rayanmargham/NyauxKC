#pragma once
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
extern volatile struct limine_module_request modules;
void populate_tmpfs_from_tar();
