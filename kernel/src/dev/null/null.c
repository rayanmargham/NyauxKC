#include "null.h"
#include "fs/vfs/vfs.h"
#include <arch/x86_64/syscalls/syscall.h>

// impl open() later on
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
struct devfsops nullops = {.rw = rw, .ioctl = ioctl};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
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
  kprintf("chilling\r\n");
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "null", VDEVICE,
                                 &vnode_devops, &res, info, NULL);
}
