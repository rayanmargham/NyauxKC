#include "keyboard.hpp"
#include "controllers/i8042/ps2.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "utils/libc.h"

static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static void open(struct vnode *curvnode, void *data, int *res,
                 struct FileDescriptorHandle *hnd);
static int close(struct vnode *curvnode, void *data,
                 struct FileDescriptorHandle *hnd);
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data);
struct devfsops kbdops = {
    .open = open, .close = close, .rw = rw, .ioctl = ioctl, .poll = poll};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res) {
  if (rw == 1) {
    
    ps2allstars *kbd = static_cast<ps2allstars*>(data);

    if (size != sizeof(nyauxps2kbdpacket)) {
      *res = EINVAL;
      return 0;
    }
    for (int i = 0; i < 256; i++) {
      ps2keyboard *k = kbd->ourguys[i];
      if (k == nullptr) {
        continue;
      }
      k->add_packet_to_ringbuf(*static_cast<nyauxps2kbdpacket*>(buffer));

    }
    
    *res = 0;
    return sizeof(nyauxps2kbdpacket);
  } else {
    ps2keyboard *kbd = static_cast<ps2keyboard *>(hnd->privatedata);
    uint64_t val = 0;
    nyauxps2kbdpacket *pac;
    int ress = get_ringbuf(kbd->buf, &val);
    pac = reinterpret_cast<nyauxps2kbdpacket*>(val);
    if (!ress) {
      *res = 0;
      return 0;
    }

    memcpy(buffer, pac, sizeof(nyauxps2kbdpacket));
    sprintf("fd %d got packet of key %d\r\n", hnd->fd, pac->keycode);
    kfree(pac, sizeof(nyauxps2kbdpacket));
    *res = 0;
    return sizeof(nyauxps2kbdpacket);
  }
}
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  return ENOSYS;
}
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data) {
  ps2keyboard *kbd = static_cast<ps2keyboard *>(get_fd(requested->fd)->privatedata);
  short cringeevents = requested->events;
  if (requested->events & POLLIN) {
    if (empty_ringbuf(kbd->buf)) {
      cringeevents &= ~(POLLIN);
    }
  }
  requested->revents = cringeevents;
  return 0;
}

extern "C" void devkbd_init(struct vfs *curvfs) {
  // struct vnode *res;
  // struct devfsinfo *info =
  //     (struct devfsinfo *)kmalloc(sizeof(struct devfsinfo));
  // info->major = 1;
  // info->minor = 1;
  // info->ops = &fbdevops;
 
  struct vnode *res;
  struct devfsinfo *info =
      static_cast<devfsinfo *>(kmalloc(sizeof(struct devfsinfo)));
  info->data = new ps2allstars;
  info->major = 10; // major ten in nyaux is i/o devices
  info->minor = 0;
  info->ops = &kbdops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char *)"keyboard",
                                 VCHRDEVICE, &vnode_devops, &res, info, NULL);

int result = i8042_init();
  if (result != 0) {
    return;
  }
}
static void open(struct vnode *curvnode, void *data, int *res,
                 struct FileDescriptorHandle *hnd) {

  kprintf("adding ps2kbd for fd %d\r\n", hnd->fd);
ps2keyboard *set = static_cast<ps2allstars*>(data)->add_one(hnd->fd);
  hnd->privatedata = set;
  *res = 0;
  return;
}
static int close(struct vnode *curvnode, void *data,
                 struct FileDescriptorHandle *hnd) {
  static_cast<ps2allstars*>(data)->remove_one(hnd->fd);
  hnd->privatedata = nullptr;
  return 0;
}