#include "fd.h"

#include "sched/sched.h"
#include "utils/hashmap.h"

uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct FileDescriptorHandle *user = item;
  return hashmap_sip((const void *)&user->fd, sizeof(int), seed0, seed1);
}
bool fd_iter(const void *item, void *udata) {
  const struct FileDescriptorHandle *user = item;
  kprintf("-> %d with offset %lu\n", user->fd, user->offset);
  return true;
}
int fd_compare(const void *a, const void *b, void *udata) {
  const struct FileDescriptorHandle *ua = a;
  const struct FileDescriptorHandle *ub = b;
  return (ua->fd != ub->fd);
}
int fddalloc(struct vnode *node) {
  struct process_t *proc = get_process_start();
  for (int i = 0; i < 256; i++) {
    if (proc->fdalloc[i] == 0) {
      proc->fdalloc[i] = 1;
      hashmap_set(proc->fds, &(struct FileDescriptorHandle){
                                 .fd = i, .offset = 0, .node = node});
      get_process_finish(proc);
      return i;
    }
  }
  get_process_finish(proc);
  return -1;
}
int fdmake(int oldfd, int fd) {
  struct FileDescriptorHandle *g = get_fd(oldfd);
  struct process_t *proc = get_process_start();
  proc->fdalloc[fd] = 1;
  hashmap_set(
      proc->fds,
      &(struct FileDescriptorHandle){
          .fd = fd, .offset = 0, .node = g->node, .dummy = true, .realhnd = g});
  get_process_finish(proc);
  return fd;
}
int fddup(int fromfd) {
  struct FileDescriptorHandle *res = get_fd(fromfd);
  int newfd = fddalloc(res->node);
  struct FileDescriptorHandle *other = get_fd(newfd);
  other->dummy = true;
  other->realhnd = res;
  return newfd;
}
struct FileDescriptorHandle *get_fd(int fd) {
  struct process_t *proc = get_process_start();
  struct FileDescriptorHandle *res =
      hashmap_get(proc->fds, &(struct FileDescriptorHandle){.fd = fd});
  if (!res) {
    get_process_finish(proc);
    return NULL;
  }
  while (res->dummy) {
    if (res->realhnd != NULL) {
      res = res->realhnd;
    }
  }
  get_process_finish(proc);
  return res;
}
void fddfree(int fd) {
  struct process_t *proc = get_process_start();

  if (proc->fdalloc[fd] == 1) {
    proc->fdalloc[fd] = 0;
    const void *e =
        hashmap_delete(proc->fds, &(struct FileDescriptorHandle){.fd = fd});
    if (e == NULL) {
      kprintf("couldnt find handle\r\n");
    }
  }
  get_process_finish(proc);
}
void duplicate_process_fd(struct process_t *from, struct process_t *to) {
  // this ASSUMES that the hashmap for from is allocated
  to->fds = hashmap_new(sizeof(struct FileDescriptorHandle), 0, 0, 0, fd_hash,
                        fd_compare, NULL, NULL);
  for (int i = 0; i < 256; i++) {
    to->fdalloc[i] = from->fdalloc[i];
    struct FileDescriptorHandle *hnd =
        (struct FileDescriptorHandle *)hashmap_get(
            from->fds, &(struct FileDescriptorHandle){.fd = i});
    if (!hnd)
      continue;
    hashmap_set(to->fds, (const struct FileDescriptorHandle *)hnd);
  }
}