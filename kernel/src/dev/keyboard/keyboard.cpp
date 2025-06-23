#include "keyboard.hpp"
#include "controllers/i8042/ps2.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
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
    
    ps2keyboard *kbd = static_cast<ps2keyboard *>(data);

    if (size != sizeof(nyauxps2kbdpacket)) {
      return EINVAL;
    }
    kbd->add_packet_to_ringbuf(*static_cast<nyauxps2kbdpacket *>(buffer));
    *res = 0;
    return sizeof(nyauxps2kbdpacket);
  } else {
    ps2keyboard *kbd = static_cast<ps2keyboard *>(data);
    subscriber *who = static_cast<subscriber *>(hnd->privatedata);
    int ress = kbd->pop_from_ringbuf();
    if (ress) {
      *res = 0;
      return 0;
    }
    nyauxps2kbdpacket *pac = who->get_packet();
    if (pac == NULL) {
      *res = 0;
      return 0;
    }
    memcpy(buffer, pac, sizeof(nyauxps2kbdpacket));
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
  ps2keyboard *kbd = static_cast<ps2keyboard *>(data);
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
  int result = i8042_init();
  if (result != 0) {
    return;
  }
  struct vnode *res;
  struct devfsinfo *info =
      static_cast<devfsinfo *>(kmalloc(sizeof(struct devfsinfo)));
  info->data = new ps2keyboard;
  info->major = 10; // major ten in nyaux is i/o devices
  info->minor = 0;
  info->ops = &kbdops;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, (char *)"keyboard",
                                 VCHRDEVICE, &vnode_devops, &res, info, NULL);
}
static void open(struct vnode *curvnode, void *data, int *res,
                 struct FileDescriptorHandle *hnd) {
  ps2keyboard *kbd = static_cast<ps2keyboard *>(data);
  subscriber *set = nullptr;
  sprintf("before operation\r\n");
  kbd->onopen(&set);
  sprintf("after opeartion\r\n");
  hnd->privatedata = set;
  *res = 0;
  return;
}
static int close(struct vnode *curvnode, void *data,
                 struct FileDescriptorHandle *hnd) {
  ps2keyboard *kbd = static_cast<ps2keyboard *>(data);
  subscriber *gonerdeltarune = static_cast<subscriber *>(hnd->privatedata);
  kbd->onclose(gonerdeltarune);
  hnd->privatedata = nullptr;
  return 0;
}