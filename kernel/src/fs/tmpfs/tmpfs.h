#pragma once
#include "utils/hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
struct tmpfsnode {
  struct vnode *node;
  char *name;
  size_t size;

  void *data; // dir entry would be stored here if it was a directory
};

struct direntry {
  struct tmpfsnode **nodes;
  size_t cnt;
};
extern struct vnodeops tmpfs_ops;
extern struct vfs_ops tmpfs_vfsops;
