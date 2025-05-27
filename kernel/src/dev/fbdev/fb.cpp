#include "fb.hpp"
#include "fs/devfs/devfs.h"
#include "fs/vfs/vfs.h"
#include "mem/kmem.h"
#include "term/term.h"
#include <cppglue/glue.hpp>
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
  // struct vnode *res;
  // struct devfsinfo *info =
  //     (struct devfsinfo *)kmalloc(sizeof(struct devfsinfo));
  // info->major = 1;
  // info->minor = 1;
  // info->ops = &fbdevops;
  kprintf("fbdev(): init\r\n");
  for (size_t i = 0; i < framebuffer_request.response->framebuffer_count; i++) {
    kprintf("fbdev(): initing buf %lu\r\n", i);
    struct vnode *res;
    struct devfsinfo *info =
        static_cast<devfsinfo *>(kmalloc(sizeof(struct devfsinfo)));
    info->major = i;
    info->minor = i + 1;
    info->ops = &fbdevops;
    char buffer[100];
    npf_snprintf(buffer, 100, "fb%lu", i);
    kprintf("attempting to create %s\r\n", buffer);
    curvfs->cur_vnode->ops->create(curvfs->cur_vnode, strdup(buffer),
                                   VCHRDEVICE, &vnode_devops, &res,
                                   static_cast<void *>(info), NULL);
  }
  // curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char *)"fb0",
  // VCHRDEVICE,
  //                                &vnode_devops, &res, info, NULL);
}
