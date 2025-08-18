#include "fd.h"

#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "utils/basic.h"
#include "utils/hashmap.h"

uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct hfd *user = item;
  return hashmap_sip((const void *)&user->fd, sizeof(int), seed0, seed1);
}
bool fd_iter(const void *item, void *udata) {
  const struct hfd*user = item;
  kprintf("-> %d \n", user->fd);
  return true;
}
int fd_compare(const void *a, const void *b, void *udata) {
  const struct hfd *ua = a;
  const struct hfd *ub = b;
  return (ua->fd != ub->fd);
}
int alloc_fd(struct FileDescriptorHandle *hnd) {
  struct process_t *proc = get_process_start();
  for (int i = 0; i < 256; i++) {
    if (proc->fdalloc[i] == 0) {
      proc->fdalloc[i] = 1;
      hashmap_set(proc->fds, &hnd);
      get_process_finish(proc);
      return i;
    }
  }
  get_process_finish(proc);
  return -1;
}
int alloc_fd_struct(struct vnode *node) {
  struct FileDescriptorHandle *hnd = kmalloc(sizeof(struct FileDescriptorHandle));


  hnd->flags = 0;
  hnd->mode = 0;
  hnd->offset = 0;
  hnd->ref = 1;
  hnd->privatedata = 0;
  hnd->node = node;
  int fd = alloc_fd(hnd);
  if (fd == -1) {
    kfree(hnd, sizeof(struct FileDescriptorHandle));
    
  } else {
    refcount_inc(&node->cnt);
  }

  return fd;
}
int fdmake(int oldfd, int fd) {
  struct FileDescriptorHandle *g = get_fd(oldfd);
  struct process_t *proc = get_process_start();
  proc->fdalloc[fd] = 1;
  hashmap_set(
      proc->fds,
      &(struct hfd){
          .fd = fd, .hnd = g});
  get_process_finish(proc);
  return fd;
}
int fddup(int fromfd) {
  struct FileDescriptorHandle *res = get_fd(fromfd);
  if (res->node) {
    refcount_inc(&res->node->cnt);
  }
  int newfd = alloc_fd_struct(res->node);

  return newfd;
}
struct FileDescriptorHandle *get_fd(int fd) {
  struct process_t *proc = get_process_start();
  struct FileDescriptorHandle *res =
      ((struct hfd*)hashmap_get(proc->fds, &(struct hfd){.fd = fd}))->hnd;
  if (!res) {
    get_process_finish(proc);
    return NULL;
  }

  get_process_finish(proc);
  return res;
}
void fddfree(int fd) {
  struct FileDescriptorHandle *ourguy = get_fd(fd);
  int maybe = refcount_dec(&ourguy->ref) + 1;
  struct process_t *proc = get_process_start();
  if (proc->fdalloc[fd] == 1 && maybe == 0) {
    proc->fdalloc[fd] = 0;
    const void *e =
        hashmap_delete(proc->fds, ourguy);
    if (e == NULL) {
      kprintf("couldnt find handle\r\n");
    }
  }
  get_process_finish(proc);
}
void duplicate_process_fd(struct process_t *from, struct process_t *to) {
  // this ASSUMES that the hashmap for from is allocated
  to->fds = hashmap_new(sizeof(struct hfd), 0, 0, 0, fd_hash,
                        fd_compare, NULL, NULL);
  for (int i = 0; i < 256; i++) {
    to->fdalloc[i] = from->fdalloc[i];
    struct FileDescriptorHandle *hnd =
        ((struct hfd*)hashmap_get(
            from->fds, &(struct hfd){.fd = i}))->hnd;
    if (!hnd)
      continue;
    refcount_inc(&hnd->ref);
    hashmap_set(to->fds, &(const struct hfd){.fd = i, .hnd = hnd});
  }
}
