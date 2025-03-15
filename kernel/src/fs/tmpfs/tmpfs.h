#pragma once
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
struct tmpfsnode {
  struct vnode *node;
  char *name;
  size_t size;
  union {
    struct direntry *direntry;
    void *data;
  };
};
struct direntry {
  struct tmpfsnode **nodes;
  size_t cnt;
};
extern struct vnodeops tmpfs_ops;
extern struct vfs_ops tmpfs_vfsops;
