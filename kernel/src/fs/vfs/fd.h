#pragma once

#include "utils/hashmap.h"
#include <sched/sched.h>
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
#ifdef __cplusplus
extern "C" {
#endif
struct FileDescriptorHandle {
  uint64_t offset;
  struct vnode *node;
  unsigned int mode;
  unsigned int flags;
  refcount_t ref;
  void *privatedata;

};
struct hfd {
  struct FileDescriptorHandle *hnd;
  int fd;
};
int fd_compare(const void *a, const void *b, void *udata);
bool fd_iter(const void *item, void *udata);
uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1);
int alloc_fd_struct(struct vnode *node);
void fddfree(int fd);
int fddup(int fromfd);
int fdmake(int oldfd, int fd);
struct FileDescriptorHandle *get_fd(int fd);
void duplicate_process_fd(struct process_t *from, struct process_t *to);
int ialloc_fd_struct(struct process_t *proc);
#ifdef __cplusplus
}
#endif
