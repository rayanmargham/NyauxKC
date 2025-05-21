#include "fb.h"
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static int poll(struct vnode *curvnode, struct pollfd *requested);
struct devfsops fbdevops = {.rw = rw, .ioctl = ioctl, .poll = poll};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res) {
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
static int poll(struct vnode *curvnode, struct pollfd *requested) {
  requested->revents = requested->events;
  return 0;
}
void devfbdev_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info =
      (struct devfsinfo *)kmalloc(sizeof(struct devfsinfo));
  info->major = 1;
  info->minor = 1;
  info->ops = &fbdevops;
  kprintf("fbdev(): init\r\n");
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char *)"fb0", VCHRDEVICE,
                                 &vnode_devops, &res, info, NULL);
}
