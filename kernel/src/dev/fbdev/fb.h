#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "fs/vfs/fd.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
extern struct devfsops fbdevops;
void devfbdev_init(struct vfs *curvfs);
#ifdef __cplusplus
}
#endif