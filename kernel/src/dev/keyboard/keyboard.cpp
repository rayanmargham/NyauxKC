#include "keyboard.hpp"
#include "controllers/i8042/i8042.h"


static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static int poll(struct vnode *curvnode, struct pollfd *requested);
struct devfsops kbdops = {.rw = rw, .ioctl = ioctl, .poll = poll};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res) {
                    return ENOSYS;
}
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  return ENOSYS;
}
static int poll(struct vnode *curvnode, struct pollfd *requested) {
  requested->revents = requested->events;
  return 0;
}

extern "C" void devkbd_init(struct vfs *curvfs) {
  // struct vnode *res;
  // struct devfsinfo *info =
  //     (struct devfsinfo *)kmalloc(sizeof(struct devfsinfo));
  // info->major = 1;
  // info->minor = 1;
  // info->ops = &fbdevops;
 i8042_init(); 
 struct vnode *res;
  struct devfsinfo *info = static_cast<devfsinfo*>(kmalloc(sizeof(struct devfsinfo)));
  info->major = 10; // major ten in nyaux is i/o devices
  info->minor = 0;
  info->ops = &kbdops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char*)"keyboard", VCHRDEVICE,
                                 &vnode_devops, &res, info, NULL);
    
}
