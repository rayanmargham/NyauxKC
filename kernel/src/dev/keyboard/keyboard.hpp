#pragma once
#include <fs/devfs/devfs.h>
#include "fs/vfs/vfs.h"
#include <mem/kmem.h>
#include <term/term.h>
#include <arch/x86_64/syscalls/syscall.h>
#include <cppglue/glue.hpp>

#ifdef __cplusplus
extern "C" {
#endif
void devkbd_init(struct vfs *curvfs);
#ifdef __cplusplus
}
#endif