#include "tty.h"
#include "flanterm/flanterm.h"
#include "fs/devfs/devfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/libc.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <stdint.h>
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
struct devfsops ttyops = {.rw = rw, .ioctl = ioctl};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw) {
  if (rw == 0) {
    struct tty *tty = data;
    int i = 0;
    for (; i < (int)size; i++) {
      char val = 0;
      get_ringbuf(tty->rx, (uint64_t *)&val);
      ((char *)buffer)[i] = val;
    }
    return i;
  } else {
    // MOST BASIC AH TTY
    flanterm_write(get_fctx(), buffer, size);
    sprintf_write(buffer, size);
    return offset + size;
  }
}
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  kprintf("tty(): request is 0x%lx\r\n", request);
  switch (request) {
  case TIOCGWINSZ:
    // usermode is requesting to get the window size of the tty
    // for now give it the size of flanterm's context
    size_t cols = 0;
    size_t rows = 0;
    struct flanterm_context *ctx = get_fctx();
    flanterm_get_dimensions(ctx, &cols, &rows);
    struct winsize size = {.ws_row = rows, .ws_col = cols};
    *(struct winsize *)result = size;
    kprintf("tty(): returning result :)\r\n");
    return 0;

    break;
  case TCGETS:
    // usermode is requesting the termios structure
    assert(data != NULL);
    struct tty *tty = data;
    *(struct termios *)arg = tty->termi;
    kprintf("tty(): returning result for termios structure :)\r\n");
    return 0;
    break;
  case TCSETS:
    assert(data != NULL);
    struct tty *ttyy = data;
    ttyy->termi = *(struct termios *)arg;
    kprintf("tty(): set values :)\r\n");
    return 0;
    break;
  }
  return ENOSYS;
}
void devtty_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  struct tty *newtty = kmalloc(sizeof(struct tty));
  // dont fucking ask
  newtty->termi.c_cc[VINTR] = VINTR;
  newtty->termi.c_cc[VQUIT] = VQUIT;
  newtty->termi.c_cc[VERASE] = VERASE;
  newtty->termi.c_cc[VKILL] = VKILL;
  newtty->termi.c_cc[VEOF] = VEOF;
  newtty->termi.c_cc[VMIN] = VMIN;
  newtty->termi.c_cc[VSWTC] = VSWTC;
  newtty->termi.c_cc[VSTART] = VSTART;
  newtty->termi.c_cc[VSTOP] = VSTOP;
  newtty->termi.c_cc[VSUSP] = VSUSP;
  newtty->termi.c_cc[VEOL] = VEOL;
  newtty->termi.c_cc[VREPRINT] = VREPRINT;
  newtty->termi.c_cc[VDISCARD] = VDISCARD;
  newtty->termi.c_cc[VWERASE] = VWERASE;
  newtty->termi.c_cc[VLNEXT] = VLNEXT;
  newtty->termi.c_cc[VEOL2] = VEOL2;
  newtty->termi.ibaud = B38400;
  newtty->termi.obaud = B38400;
  newtty->termi.c_iflag = ICRNL | IXON;
  newtty->termi.c_oflag = OPOST | ONLCR;
  newtty->termi.c_cflag = CS8 | CREAD | HUPCL;
  newtty->termi.c_lflag = ECHOK | ICANON;
  newtty->tx = init_ringbuf(255);
  newtty->rx = init_ringbuf(255);
  info->major = 4;
  info->minor = 0;
  info->ops = &ttyops;
  info->data = newtty;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "tty", VDEVICE,
                                 &vnode_devops, &res, info, NULL);

  struct FileDescriptorHandle *hnd1 = get_fd(0);
  struct FileDescriptorHandle *hnd2 = get_fd(1);
  struct FileDescriptorHandle *hnd3 = get_fd(2);
  assert(res != NULL);
  hnd1->node = res;
  hnd2->node = res;
  hnd3->node = res;
}