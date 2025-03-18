#include "null.h"
#include "fs/vfs/vfs.h"

// impl open() later on
struct devfsops nullops = {.rw = rw};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
  if (rw) {
    return 0;
  } else {
    return size;
  }
}
void devnull_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  info->major = 1;
  info->minor = 0;
  info->ops = &nullops;
  kprintf("chilling\r\n");
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "null", VDEVICE,
                                 &vnode_devops, &res, info);
}
