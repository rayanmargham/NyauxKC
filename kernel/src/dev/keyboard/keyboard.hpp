#pragma once
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
class subscriber {
public:
  ring_buf *buf;
  subscriber *next;
  subscriber() {
    buf = init_ringbuf(100);
    next = nullptr;
  }
  nyauxps2kbdpacket *get_packet() {
    uint64_t re = 0;
    if (get_ringbuf(buf, &re) != 1) {
        return nullptr;
    }
    return reinterpret_cast<nyauxps2kbdpacket*>(re);
  }
  void take_packet(nyauxps2kbdpacket *pack) { put_ringbuf(buf, reinterpret_cast<uint64_t>(pack)); }
  void broadcast_packet(nyauxps2kbdpacket *pack) {
    take_packet(pack);
    subscriber *cur = next;
    while (cur) {
      cur->take_packet(pack);
      cur = cur->next;
    }
  }
};
class ps2keyboard {
public:
  ring_buf *buf;
  subscriber *head;
  ps2keyboard() {
    kprintf("initing ps2 device\r\n");
    buf = init_ringbuf(100);
    head = nullptr;
  }
  void add_packet_to_ringbuf(nyauxps2kbdpacket pack) {
    nyauxps2kbdpacket *neew =
        static_cast<nyauxps2kbdpacket *>(kmalloc(sizeof(nyauxps2kbdpacket)));
    *neew = pack;
    put_ringbuf(buf, reinterpret_cast<uint64_t>(neew));
  }
  int pop_from_ringbuf() {
    uint64_t re = 0;
    if (get_ringbuf(buf, &re) != 1) {
      return -1;
    }
    nyauxps2kbdpacket *epic = reinterpret_cast<nyauxps2kbdpacket *>(re);
    head->broadcast_packet(epic);
    return 0;
  }
  void onopen(subscriber **setptr) {
    *setptr = new subscriber;
    if (head == nullptr) {
        head = *setptr; }
    else {
        subscriber *cur = head;
    subscriber *prev = nullptr;
    while (cur) {
        sprintf("cur is %p, prev is %p, setptr is %p\r\n", cur, prev, *setptr);
        
        prev = cur;
        cur = cur->next;
    }
    prev->next = *setptr;
    }
  }
  void onclose(subscriber *whotogetridof) {
    subscriber *cur = head;
    subscriber *prev = nullptr;
    while (cur) {
        if (cur == whotogetridof) {
            sprintf("begone\r\n");
            if (prev != nullptr)
                prev->next = cur->next;
            delete cur;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    if (whotogetridof == head) {
        delete head;
    } else if (whotogetridof == nullptr){
        panic("thats crazyyy %p\r\n", whotogetridof);
    }
  }
};
#endif