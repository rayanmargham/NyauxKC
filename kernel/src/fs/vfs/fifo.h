#pragma once
#include "fs/vfs/fd.h"
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>

struct fifo {
    struct ring_buf *ballbuffer;
    spinlock_t readlock;
    spinlock_t writelock;
    _Atomic int read_count;
    _Atomic int write_count;
    bool namedpipe;
};

void fifo_open(struct FileDescriptorHandle *hnd);
void fifo_close(struct FileDescriptorHandle *hnd);
size_t fifo_read(struct fifo *hoot, uint8_t *buffer, size_t size, int *res, bool blocking);
void fifo_close(struct FileDescriptorHandle *hnd);
size_t fifo_write(struct fifo *hoot, uint8_t *buffer, size_t size, int *res, bool blocking);
void pipe_create(int output[2]);