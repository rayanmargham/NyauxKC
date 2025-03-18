#include "tty.h"
#include "fs/devfs/devfs.h"
struct devfsops ttyops = {.rw = rw};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
  if (rw) {
    return 0;
  } else {

    return size;
  }
}
void devtty_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  info->major = 4;
  info->minor = 0;
  info->ops = &ttyops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "tty", VREG, &vnode_devops,
                                 &res, info);
}