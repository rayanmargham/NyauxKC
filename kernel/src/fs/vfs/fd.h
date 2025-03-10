#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>

#include "utils/hashmap.h"
struct FileDescriptionHandle
{
	int fd;
	uint64_t offset;
	// ops here
};
int fd_compare(const void* a, const void* b, void* udata);
bool fd_iter(const void* item, void* udata);
uint64_t fd_hash(const void* item, uint64_t seed0, uint64_t seed1);
