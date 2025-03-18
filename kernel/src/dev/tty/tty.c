#include "tty.h"
#include "fs/devfs/devfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "term/term.h"
struct devfsops ttyops = {.rw = rw};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
  if (rw == 0) {
    return offset + size;
  } else {
    // MOST BASIC AH TTY
    flanterm_write(get_fctx(), buffer, size);
    sprintf_write(buffer, size);
    return offset + size;
  }
}
void devtty_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  info->major = 4;
  info->minor = 0;
  info->ops = &ttyops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "tty", VDEVICE,
                                 &vnode_devops, &res, info);
  struct FileDescriptorHandle *hnd1 = get_fd(0);
  struct FileDescriptorHandle *hnd2 = get_fd(1);
  struct FileDescriptorHandle *hnd3 = get_fd(2);
  assert(res != NULL);
  hnd1->node = res;
  hnd2->node = res;
  hnd3->node = res;
}