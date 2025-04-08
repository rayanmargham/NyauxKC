#include "fb.h"
#include <arch/x86_64/syscalls/syscall.h>
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
struct devfsops fbdevops = {.rw = rw, .ioctl = ioctl};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
  sprintf("fbdev: wants to write\r\n");
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
void devfbdev_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  info->major = 1;
  info->minor = 1;
  info->ops = &fbdevops;
  kprintf("fbdev: init\r\n");
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "fb0", VCHRDEVICE,
                                 &vnode_devops, &res, info, NULL);
}
