#pragma once
#include "utils/hashmap.h"
#include <sched/sched.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
struct FileDescriptorHandle {
  int fd;
  uint64_t offset;
  bool dummy; // if true use realhnd
  struct vnode *node;
  unsigned int mode;
  unsigned int flags;
  struct FileDescriptorHandle *realhnd;
  // ops here
};
int fd_compare(const void *a, const void *b, void *udata);
bool fd_iter(const void *item, void *udata);
uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1);
int fddalloc(struct vnode *node);
void fddfree(int fd);
int fddup(int fromfd);
int fdmake(int oldfd, int fd);
struct FileDescriptorHandle *get_fd(int fd);
void duplicate_process_fd(struct process_t *from, struct process_t *to);