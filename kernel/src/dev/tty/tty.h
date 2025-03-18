#pragma once
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw);
extern struct devfsops ttyops;
struct tty_device {
  void (*write)(char *shit, size_t size);
};
struct tty {
  struct ring_buf *rx;
  struct ring_buf *tx;
  struct tty_device *device;
};
void devtty_init(struct vfs *curvfs);