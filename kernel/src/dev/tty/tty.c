#include "tty.h"
#include "controllers/i8042/ps2.h"
#include "flanterm/src/flanterm.h"
#include "fs/devfs/devfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"
#include "utils/cmdline.hpp"
#include "utils/libc.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <stdint.h>

static size_t rw(struct vnode *curvnode, void *data, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd, int *ret);
static int ioctl(struct vnode *curvnode, void *data, unsigned long request,
                 void *arg, void *result);
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data);
static void open(struct vnode *curvnode, void *data, int *res, struct FileDescriptorHandle *hnd);
static int close(struct vnode *curvnode, void *data, struct FileDescriptorHandle *hnd);
struct devfsops ttyops = {.open = open, .close = close, .rw = rw, .ioctl = ioctl, .poll = poll};
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
  *(void **)result = 0;
  switch (request) {
  case TIOCGWINSZ:
    // usermode is requesting to get the window size of the tty
    // for now give it the size of flanterm's context
    size_t cols = 0;
    size_t rows = 0;
    struct flanterm_context *ctx = get_fctx();
    flanterm_get_dimensions(ctx, &cols, &rows);
    sprintf("tty(): cols %lu rows %lu\r\n", cols, rows);
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

    ttyy->termi = *(struct termios *)arg;

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
static int poll(struct vnode *curvnode, struct pollfd *requested, void *data) {
  sprintf("tty(): poll() called on me, events are %d\r\n", requested->events);
  requested->revents = requested->events; // for now
  return 0;
}
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
  kprintf_log(FATAL, "WHAT??\r\n");
  exit_thread();
}
void ps2_put_input() {
  struct vnode *lah = NULL;
  // just hope the underlying device recongizes that the kernel is asking this shit
  vfs_lookup(NULL, "/dev/keyboard", &lah);

  if (lah == NULL)
    exit_thread();
  int res = 0;
  int fd = lah->ops->open(lah, O_RDONLY, 0644, &res);
  if (res != 0)
    exit_thread();
  while (true) {
    if (curtty) {
      if (curtty->rx) {
        struct pollfd yes = {fd, POLLIN, 0}; // fd -1 is a hack
        lah->ops->poll(lah, &yes);
        if (yes.revents & POLLIN) {
        // spinlock_lock(&curtty->rxlock);
        
        nyauxps2kbdpacket pac;
        int res = 0;
        lah->ops->rw(lah, 0, sizeof(nyauxps2kbdpacket), &pac, 0, get_fd(fd), &res);
        if (res != 0) {
          sprintf("exitting\r\n");
          // spinlock_unlock(&curtty->rxlock);
          exit_thread();
        }
        if (pac.flags == PRESSED) {
        put_ringbuf(curtty->rx, pac.ascii); };
        // spinlock_unlock(&curtty->rxlock);
yes.revents = 0;
      } 
      

      }
    }
  }
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
  char *optionchk[3] = {
    "serial_input",
    "serial_ttyinput",
    "serial.input"
  };
  struct cmdlineobj *obj = NULL;
  for (int i = 0; i < 3; i++) {
    void *chk = look_for_option(optionchk[i]);
    if (chk != NULL) {
      obj = (struct cmdlineobj*)chk;
    }
  }
  if (obj == NULL) {
    kprintf_log(STATUSFAIL, "not using serial input due to cmdline option...\r\n");
    goto mrrp;
  }
  if (obj->type == Bool) {
    struct process_t *p = get_process_start();

  create_kthread((uint64_t)serial_put_input, p, p->pid);
  get_process_finish(p);
  goto yea;
  }
  mrrp:
    struct process_t *p = get_process_start();
    create_kthread((uint64_t)ps2_put_input, p, p->pid + 1);
 get_process_finish(p);
 yea:
}
static void open(struct vnode *curvnode, void *data, int *res, struct FileDescriptorHandle *hnd) {
  *res = 0;
}
static int close(struct vnode *curvnode, void *data, struct FileDescriptorHandle *hnd) {
  return 0;
}