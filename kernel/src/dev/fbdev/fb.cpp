#include "fb.hpp"
#include "fs/devfs/devfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "mem/kmem.h"
#include "term/term.h"
#include <cppglue/glue.hpp>

static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static void open(struct vnode *curvnode, void *data, int *res, struct FileDescriptorHandle *hnd);
static int close(struct vnode *curvnode, void *data, struct FileDescriptorHandle *hnd);
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data);
struct devfsops fbdevops = {.open = open, .close = close,.rw = rw, .ioctl = ioctl, .poll = poll};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res) {

  FBDev *owner = static_cast<class FBDev *>(data);
  if (!rw) {
    return owner->read(buffer, offset, size, hnd, res);
  } else {
    return owner->write(buffer, offset, size, hnd, res);
  }
}
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  FBDev *owner = static_cast<class FBDev *>(data);
  return owner->ioctl(data, request, arg, result);
}
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data) {
  requested->revents = requested->events;
  return 0;
}

extern "C" void devfbdev_init(struct vfs *curvfs) {
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
    info->data = new FBDev(
        i,
        new LimineFrameBuffer(framebuffer_request.response->framebuffers[i]));
    char buffer[100];
    npf_snprintf(buffer, 100, "fb%lu", i);
    curvfs->cur_vnode->ops->create(curvfs->cur_vnode, strdup(buffer),
                                   VCHRDEVICE, &vnode_devops, &res,
                                   static_cast<void *>(info), NULL);
  }
  // curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char *)"fb0",
  // VCHRDEVICE,
  //                                &vnode_devops, &res, info, NULL);
}
static void open(struct vnode *curvnode, void *data, int *res, struct FileDescriptorHandle *hnd) {
  *res = 0;
};
static int close(struct vnode *curvnode, void *data, struct FileDescriptorHandle *hnd) {
  return 0;
};