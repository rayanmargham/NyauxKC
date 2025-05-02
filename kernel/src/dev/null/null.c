#include "null.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include <arch/x86_64/syscalls/syscall.h>

// impl open() later on
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd, int *res);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static int poll(struct vnode *curvnode, struct pollfd *requested);
struct devfsops nullops = {.rw = rw, .ioctl = ioctl, .poll = poll};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd, int *res) {
  if (rw) {
    return 0;
  } else {
    return size;
  }
}
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  return ENOSYS;
}
void devnull_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  info->major = 1;
  info->minor = 0;
  info->ops = &nullops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "null", VCHRDEVICE,
                                 &vnode_devops, &res, info, NULL);
}
static int poll(struct vnode *curvnode, struct pollfd *requested) {
  requested->revents = requested->events;
  return 0;
}