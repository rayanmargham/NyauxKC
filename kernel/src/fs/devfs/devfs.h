#pragma once
#include "fs/vfs/fd.h"
#include <fs/vfs/vfs.h>
#include <mem/kmem.h>
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>

struct devfsops {
  size_t (*rw)(struct vnode *curvnode, void *data, size_t offset, size_t size,
               void *buffer, int rw, struct FileDescriptorHandle *hnd, int *res);
  int (*ioctl)(struct vnode *curvnode, void *data, unsigned long request,
               void *arg, void *result);
  int (*poll)(struct vnode *curvnode, struct pollfd *requested);
};
struct devfsinfo {
  uint8_t major;
  uint8_t minor;
  struct devfsops *ops;
  void *data; // anything the device file wants to store
  // .... anyhting else
};
struct devfsnode {
  struct vnode *curvnode;
  struct devfsinfo *info;
  struct devfsdirentry *direntry;
  char *name;
};
extern struct vnodeops vnode_devops;
extern struct vfs_ops vfs_devops;
void devfs_init(struct vfs *curvfs);

struct devfsdirentry {
  struct devfsnode **nodes;
  size_t cnt;
};
