#include "fifo.h"
#include "arch/x86_64/syscalls/syscall.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "sched/sched.h"
#include "utils/basic.h"
#include "utils/libc.h"

void fifo_open(struct FileDescriptorHandle *hnd) {
	int accessmode = hnd->flags & O_ACCMODE;
	struct fifo *balljaw = hnd->node->data;
	switch (accessmode) {
	case O_RDONLY:
		balljaw->read_count++;
		while (balljaw->write_count == 0) {
			sched_yield();
		}
		break;
	case O_WRONLY:
		balljaw->write_count++;
		while (balljaw->read_count == 0) {
			sched_yield();
		}
		break;
	case O_RDWR:
		balljaw->read_count++;
		balljaw->write_count++;
		break;
	}
	refcount_inc(&hnd->node->cnt);
}
void fifo_close(struct FileDescriptorHandle *hnd) { 
    sprintf("attempt at fifo close on fd %d, vnode %p, FD PTR THIS IS IMPORTANT %p\r\n", hnd->fd, hnd->node, hnd);
    if (hnd->node == NULL) {
        panic("we already dealt with ya\r\n");
    }
    struct fifo *feet = hnd->node->data;
    switch (hnd->mode) {
        case O_RDONLY:
            feet->read_count--;

            break;
        case O_WRONLY:
            feet->write_count--;

            break;
        case O_RDWR:
            feet->read_count--;
            feet->write_count--;
            break;
    }

    if (!feet->namedpipe) {
        sprintf("i have run\r\n");

        int rah = refcount_dec(&hnd->node->cnt);
        if (rah == 0) {
        kfree(hnd->node->data, sizeof(struct fifo));
        kfree(hnd->node, sizeof(struct vnode));
        hnd->node = NULL;
        }
    }

    fddfree(hnd->fd);
}
void pipe_create(int output[2]) {
	struct vnode *newnode = kmalloc(sizeof(struct vnode));
	struct fifo *feet = kmalloc(sizeof(struct fifo));
	newnode->data = feet;
	newnode->v_type = VFIFO;
	output[0] = fddalloc(newnode);
	output[1] = fddalloc(newnode);
	struct FileDescriptorHandle *read = get_fd(output[0]);
	struct FileDescriptorHandle *write = get_fd(output[1]);
	read->node = newnode;
	read->mode = O_RDONLY;
	write->node = newnode;
	write->mode = O_WRONLY;
	feet->read_count++;
	feet->write_count++;
    feet->namedpipe = false;
	feet->ballbuffer = init_ringbuf(1024);
	refcount_inc(&newnode->cnt);
	refcount_inc(&newnode->cnt);
}
// TODO
// if a pipe was in a middle of a write and the read end was closed, you must
// stop mid write and return EPIPE
size_t fifo_read(struct fifo *hoot, uint8_t *buffer, size_t size, int *res,
				 bool blocking) {
	size_t data_read = 0;
	if (size == 0) {
		*res = 0;
		return 0;
	}
	if (blocking == false) {
		uint64_t val = 0;
		spinlock_lock(&hoot->readlock);
		if (get_ringbuf(hoot->ballbuffer, &val) == 0) {

			spinlock_unlock(&hoot->readlock);
			if (hoot->write_count == 0) {
				*res = 0;
				return 0;
			}
			*res = EAGAIN;
			return 0;
		} else {
			spinlock_unlock(&hoot->readlock);
			*buffer = val;
			data_read += 1;
		}
	} else {
		uint64_t val = 0;
		spinlock_lock(&hoot->readlock);
		while (get_ringbuf(hoot->ballbuffer, &val) == 0) {

			spinlock_unlock(&hoot->readlock);
			if (hoot->write_count == 0) {
				*res = 0;
				return 0;
			}
			sched_yield();

			spinlock_lock(&hoot->readlock);
		}
		spinlock_unlock(&hoot->readlock);
		data_read += 1;
		*buffer = val;
	}
	while (data_read < size) {
		spinlock_lock(&hoot->readlock);

		uint64_t val = 0;
		if (get_ringbuf(hoot->ballbuffer, &val) == 0) {
			spinlock_unlock(&hoot->readlock);

			break;
		}
		buffer[data_read] = val;
		data_read += 1;
		spinlock_unlock(&hoot->readlock);
	}
	*res = 0;
	return data_read;
}

size_t fifo_write(struct fifo *hoot, uint8_t *buffer, size_t size, int *res,
				  bool blocking) {
	size_t data_writed = 0;
	if (size == 0) {
		*res = 0;
		return 0;
	}

	if (blocking == false) {
		spinlock_lock(&hoot->readlock);
		if (put_ringbuf(hoot->ballbuffer, *buffer) == 0) {
			spinlock_unlock(&hoot->readlock);
if (hoot->read_count == 0) {
				*res = EPIPE;
				return -1;
			}
			*res = EAGAIN;
			return 0;
		} else {
			spinlock_unlock(&hoot->readlock);
			data_writed += 1;
		}
	} else {
		spinlock_lock(&hoot->readlock);

		while (put_ringbuf(hoot->ballbuffer, *buffer) == 0) {

			spinlock_unlock(&hoot->readlock);
if (hoot->read_count == 0) {
				*res = EPIPE;
				return -1;
			}
			sched_yield();

			spinlock_lock(&hoot->readlock);
		}
		spinlock_unlock(&hoot->readlock);
		data_writed += 1;
	}
	while (data_writed < size) {
		spinlock_lock(&hoot->readlock);
		if (put_ringbuf(hoot->ballbuffer, buffer[data_writed]) == 0) {

			spinlock_unlock(&hoot->readlock);

			break;
		}
		data_writed += 1;
		spinlock_unlock(&hoot->readlock);
	}

	*res = 0;
	return data_writed;
}