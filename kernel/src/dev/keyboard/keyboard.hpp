#pragma once
#include "fs/vfs/fd.h"
#include <arch/x86_64/syscalls/syscall.h>
#include <controllers/i8042/ps2.h>
#include <cppglue/glue.hpp>
#include <fs/devfs/devfs.h>
#include <fs/vfs/vfs.h>
#include <mem/kmem.h>
#include <term/term.h>
#include <utils/libc.h>

#ifdef __cplusplus
extern "C" {
#endif
  void devkbd_init(struct vfs *curvfs);

#ifdef __cplusplus
}

class ps2keyboard {
public:
  int fd;
  ring_buf *buf;
  ps2keyboard(int fdd) {
    fd = fdd;
    kprintf("initing ps2 device\r\n");
    buf = init_ringbuf(100);
  }
  void add_packet_to_ringbuf(nyauxps2kbdpacket pack) {
    nyauxps2kbdpacket *neew =
        static_cast<nyauxps2kbdpacket *>(kmalloc(sizeof(nyauxps2kbdpacket)));
    *neew = pack;
    
    put_ringbuf(buf, reinterpret_cast<uint64_t>(neew));
  }
  nyauxps2kbdpacket *noconsumeget() {
    uint64_t ok = 0;
    int res = get_without_consumeringbuf(buf, &ok);
    if (!res) {
      return nullptr;
    }
    return reinterpret_cast<nyauxps2kbdpacket*>(ok);
  }
  void bye() {
    uint64_t he;
    while (get_ringbuf(buf, &he)) {
      nyauxps2kbdpacket *pac = reinterpret_cast<nyauxps2kbdpacket*>(he);
      kfree(pac, sizeof(nyauxps2kbdpacket));
    }
  }
~ps2keyboard() {
  sprintf("kbd: going bai bai\r\n");
  bye();
  kfree(buf->buf, align_up(100 * sizeof(uint64_t), 8));
  kfree(buf, sizeof(ring_buf));
}
  void onopen() {
        
  }
  void onclose() {
 }
};
class ps2allstars {
  public:
ps2keyboard *ourguys[256] = {};
ps2allstars() {
  for (int i = 0; i < 256; i++) {
    ourguys[i] = nullptr;
  }
}
ps2keyboard *get_from_fd(int fd) {
  return ourguys[fd];
}
ps2keyboard *add_one(int fd) {
  ps2keyboard *meow = new ps2keyboard(fd);
  ourguys[fd] = meow;
  return meow;
}
void remove_one(int fd) {
  ps2keyboard *ok = ourguys[fd];
  delete ok;
  ourguys[fd] = NULL;
}
};


#endif