#include "tty.h"
#include "flanterm/flanterm.h"
#include "fs/devfs/devfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"
#include "utils/libc.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <stdint.h>

static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd, int *ret);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
struct devfsops ttyops = {.rw = rw, .ioctl = ioctl};
static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd, int *ret) {
  if (rw == 0) {
    struct tty *tty = data;
    // TODO check non blockingw
    // // kprintf("vmin is %d and vtime is %d\r\n", tty->termi.c_cc[VMIN],
    //         tty->termi.c_cc[VTIME]);
    // return a character
    int ok = size < 1 ? size : 1;
    int i = 0;
    for (; i < (int)ok; i++) {
    restart1:
      uint64_t val = 0;

      spinlock_lock(&tty->rxlock);
      int res = get_ringbuf(tty->rx, &val);
      if (res == 0 &&
          ((hnd->flags & O_NONBLOCK) ||
           (tty->termi.c_cc[VMIN] == 0 && tty->termi.c_cc[VTIME] == 0)) == false) {
        // kprintf("blocking\r\n");
        spinlock_unlock(&tty->rxlock);
        sched_yield();
        goto restart1;
      } else if (res == 0 && i == 0 && !(tty->termi.c_lflag & ICANON)) {
        spinlock_unlock(&tty->rxlock);
        return 1;
        break;
      } else if (res == 0) {
        spinlock_unlock(&tty->rxlock);
        break;
      }
      if ((char)val == '\n' && tty->termi.c_lflag & ICANON) {
        break;
      }
      // sprintf("got %c at idx %d\r\n", (char)val, i);
      ((char *)buffer)[i] = (char)val;
      spinlock_unlock(&tty->rxlock);
    }
    if (tty->termi.c_lflag & ECHO) {
      flanterm_write(get_fctx(), buffer, size);
    }
    // sprintf("idx: %d\r\n", i);
    return i;
  } else {
    // MOST BASIC AH TTY
    struct tty *tty = data;
    if (hnd->flags & O_APPEND) {
      offset = ringbuf_size(tty->tx);
      resize_ringbuf(tty->tx, offset + size);
    }
    char *him = buffer;

    flanterm_write(get_fctx(), buffer, size);
    sprintf_write(buffer, size);

    return offset + size;
  }
}

static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result) {
  sprintf("tty(): request is 0x%lx\r\n", request);
  switch (request) {
  case TIOCGWINSZ:
    // usermode is requesting to get the window size of the tty
    // for now give it the size of flanterm's context
    size_t cols = 0;
    size_t rows = 0;
    struct flanterm_context *ctx = get_fctx();
    flanterm_get_dimensions(ctx, &cols, &rows);
    struct winsize size = {.ws_row = rows, .ws_col = cols};
    *(struct winsize *)arg = size;
    return 0;

    break;
  case TCGETS:
    // usermode is requesting the termios structure
    assert(data != NULL);
    struct tty *tty = data;
    if (tty->termi.c_lflag & ICANON) {
      sprintf("tty(): canonical mode is enabled\r\n");
    }
    *(struct termios *)arg = tty->termi;
    return 0;
    break;
  case TCSETS:
    assert(data != NULL);
    struct tty *ttyy = data;
    sprintf("tty(): before c_lflags 0x%x, c_iflag 0x%x c_oflags 0x%x c_cflags 0x%x\r\n", ttyy->termi.c_lflag,
            ttyy->termi.c_iflag, ttyy->termi.c_oflag, ttyy->termi.c_cflag);
    ttyy->termi = *(struct termios *)arg;
    sprintf("tty(): after c_lflags 0x%x, c_iflag 0x%x c_oflags 0x%x c_cflags 0x%x\r\n", ttyy->termi.c_lflag,
            ttyy->termi.c_iflag, ttyy->termi.c_oflag, ttyy->termi.c_cflag);
    sprintf("tty(): set values :)\r\n");
    return 0;
    break;
  case TIOCGPGRP:

    // Get the process group ID of the foreground process group on
    // this terminal.
    // no fuck off bash
    return ENOSYS;
    break;
  case TIOCSPGRP:

    return ENOSYS;
    break;
  default:
    sprintf("tty(): unsupported tty request 0x%lx\r\n", request);
    break;
  }

  return ENOSYS;
}
static struct tty *curtty = NULL;
extern bool serial_data_ready();
void serial_put_input() {
  inb(0x3F8);
  while (true) {
    if (serial_data_ready()) {
      unsigned char got = (unsigned char)inb(0x3F8);
      if (curtty) {

        if (curtty->rx) {
          spinlock_lock(&curtty->rxlock);
          put_ringbuf(curtty->rx, (uint64_t)got);
          spinlock_unlock(&curtty->rxlock);
        }
      }
    }
  }
  exit_thread();
}
void devtty_init(struct vfs *curvfs) {
  struct vnode *res;
  struct devfsinfo *info = kmalloc(sizeof(struct devfsinfo));
  struct tty *newtty = kmalloc(sizeof(struct tty));
  // dont fucking as
  newtty->termi.c_cc[VINTR] = VINTR;
  newtty->termi.c_cc[VQUIT] = VQUIT;
  newtty->termi.c_cc[VERASE] = VERASE;
  newtty->termi.c_cc[VKILL] = VKILL;
  newtty->termi.c_cc[VEOF] = VEOF;
  newtty->termi.c_cc[VMIN] = 1;
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
  newtty->termi.c_lflag = ECHO | ICANON;
  newtty->tx = init_ringbuf(255);
  newtty->rx = init_ringbuf(255);
  curtty = newtty;
  info->major = 4;
  info->minor = 0;
  info->ops = &ttyops;
  info->data = newtty;
  curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "tty", VCHRDEVICE,
                                 &vnode_devops, &res, info, NULL);

  struct FileDescriptorHandle *hnd1 = get_fd(0);
  struct FileDescriptorHandle *hnd2 = get_fd(1);
  struct FileDescriptorHandle *hnd3 = get_fd(2);
  assert(res != NULL);
  hnd1->node = res;
  hnd2->node = res;
  hnd3->node = res;
  struct process_t *p = get_process_start();
  create_kthread((uint64_t)serial_put_input, p, 4);
  get_process_finish(p);
}