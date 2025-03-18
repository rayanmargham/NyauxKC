#include "fd.h"

#include "sched/sched.h"

uint64_t fd_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct FileDescriptionHandle *user = item;
  return hashmap_sip((const void *)&user->fd, sizeof(int), seed0, seed1);
}
bool fd_iter(const void *item, void *udata) {
  const struct FileDescriptionHandle *user = item;
  kprintf("-> %d with offset %lu\n", user->fd, user->offset);
  return true;
}
int fd_compare(const void *a, const void *b, void *udata) {
  const struct FileDescriptionHandle *ua = a;
  const struct FileDescriptionHandle *ub = b;
  return (ua->fd != ub->fd);
}
int fddalloc(struct vnode *node) {
  struct process_t *proc = get_process_start();
  for (int i = 0; i < 256; i++) {
    if (proc->fdalloc[i] == 0) {
      proc->fdalloc[i] = 1;
      hashmap_set(proc->fds, &(struct FileDescriptionHandle){
                                 .fd = i, .offset = 0, .node = node});
      get_process_finish(proc);
      return i;
    }
  }
  get_process_finish(proc);
  return -1;
}
struct FileDescriptionHandle *get_fd(int fd) {
  struct process_t *proc = get_process_start();
  struct FileDescriptionHandle *res =
      hashmap_get(proc->fds, &(struct FileDescriptionHandle){.fd = fd});
  get_process_finish(proc);
  return res;
}
void fddfree(int fd) {
  struct process_t *proc = get_process_start();

  if (proc->fdalloc[fd] == 1) {
    proc->fdalloc[fd] = 0;
    kprintf("giving %d\r\n", fd);
    size_t iter = 0;
    void *item;
    const void *e =
        hashmap_delete(proc->fds, &(struct FileDescriptionHandle){.fd = fd});
    if (e == NULL) {
      kprintf("couldnt find handle\r\n");
    }
  }
  get_process_finish(proc);
}
