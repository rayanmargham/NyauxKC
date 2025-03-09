#pragma once
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
static size_t rw(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw);
struct devfsops nullops = {.rw = rw};
