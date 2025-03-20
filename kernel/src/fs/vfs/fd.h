#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>

#include "utils/hashmap.h"
struct FileDescriptorHandle {
  int fd;
  uint64_t offset;
  bool dummy; // if true use realhnd
  struct vnode *node;
  struct FileDescriptorHandle *realhnd;
  // ops here
};
int fd_compare(const void *a, const void *b, void *udata);
bool fd_iter(const void *item, void *udata);
uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1);
int fddalloc(struct vnode *node);
void fddfree(int fd);
int fddup(int fromfd);
struct FileDescriptorHandle *get_fd(int fd);