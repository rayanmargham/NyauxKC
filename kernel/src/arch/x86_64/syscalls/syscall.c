#include "syscall.h"
#include "../instructions/instructions.h"
#include "Mutexes/seqlock.h"
#include "arch/arch.h"
#include "elf/symbols/symbols.h"
#include "fs/devfs/devfs.h"
#include "fs/tmpfs/tmpfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/fifo.h"
#include "fs/vfs/vfs.h"
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"
#include <stdint.h>
#include <timers/timer.hpp>
#define SYSCALLENT __asm__ volatile("sti");
#define SYSCALLEXIT __asm__ volatile("cli");
#define UNIMPL return (struct __syscall_ret){.ret = 0, .errno = ENOSYS};
struct __syscall_ret syscall_exit(int exit_code) {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();
	sprintf("syscall_exit(): exiting pid %lu, exit_code %d\r\n",
			cpu->cur_thread->proc->pid, exit_code);
	if (cpu->cur_thread->proc->pid == 0 || cpu->cur_thread->proc->pid == 1) {
		kprintf_log(FATAL, "init process destroyed\r\n");

		exit_thread();
	}
	cpu->cur_thread->state = ZOMBIE;
	exit_process(exit_code);
	// kill all threads TODO
	sched_yield();
	panic("shouldn't be here");
	// return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_debug(char *string, size_t length) {
	char *buffer = kmalloc(1024);
	memcpy(buffer, string, length);
	buffer[length] = '\0';
	sprintf("userland: %s\r\n", buffer);
	return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_setfsbase(uint64_t ptr) {
	sprintf("syscall_setfsbase()\r\n");
	struct per_cpu_data *cpu = arch_get_per_cpu_data();
	cpu->cur_thread->arch_data.fs_base = ptr;
	wrmsr(0xC0000100, cpu->cur_thread->arch_data.fs_base);
	return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_mmap(void *hint, size_t size, int prot, int flags,
								  int fd, size_t offset) {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();
	sprintf("syscall_mmap(): size %lu flags %x, hint %p\r\n", size, flags,
			hint);
	if (flags & MAP_ANONYMOUS) {
		if (hint != 0) {

			uint64_t shit = (uint64_t)uvmm_region_alloc_fixed(
				cpu->cur_thread->proc->cur_map, (uint64_t)hint, size, false);

			return (struct __syscall_ret){shit, 0};
		}
		uint64_t shit = (uint64_t)uvmm_region_alloc_demend_paged(
			cpu->cur_thread->proc->cur_map, size);

		return (struct __syscall_ret){(uint64_t)shit, 0};
	}
	sprintf("saying enosys");

	return (struct __syscall_ret){-1, ENOSYS};
}
struct __syscall_ret syscall_free(void *pointer, size_t size) {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();

	sprintf("syscall_free(): freeing %lu\r\n", size);
	uvmm_region_dealloc(cpu->cur_thread->proc->cur_map, pointer);

	return (struct __syscall_ret){-0, 0};
}
struct __syscall_ret syscall_openat(int dirfd, const char *path, int flags,
									unsigned int mode) {
	sprintf("syscall_openat(): opening %s from thread %lu with flags %d, dirfd %d\r\n",
			path, arch_get_per_cpu_data()->cur_thread->tid, flags, dirfd);

	struct vnode *node = NULL;
	if (dirfd == AT_FDCWD) {
		struct process_t *proc = get_process_start();
		node = proc->cwd;
		get_process_finish(proc);

	} else {
		struct FileDescriptorHandle *hnd = get_fd(dirfd);
		if (hnd == NULL) {

			return (struct __syscall_ret){.ret = -1, .errno = EBADF};
		}
		node = hnd->node;
	}
	struct vnode *retur = NULL;
	int res = vfs_lookup(node, path, &retur);
	if (res != 0) {

		return (struct __syscall_ret){.ret = -1, .errno = ENOENT};
	}
	res = 0;
	int fd = -1;
	switch (mode) {
	case O_DIRECTORY:
		if (retur->v_type == VREG) {
			return (struct __syscall_ret){.ret = -1, .errno = ENOTDIR};
		} else if (retur->v_type == VDIR) {
			return (struct __syscall_ret){.ret = -1, .errno = ENOTSUP};
		}
		break;
	default:

		if (retur->v_type == VBLKDEVICE || retur->v_type == VCHRDEVICE) {
			fd = retur->ops->open(retur, flags, mode, &res);
		} else {
			fd = retur->ops->open(retur, flags, mode, &res);
		}
		if (res != 0) {

			return (struct __syscall_ret){.ret = -1, .errno = res};
		}
		break;
	}
	struct FileDescriptorHandle *h = get_fd(fd);
	sprintf("KANKER_openat(): %d\r\n", fd);
	h->flags = flags;
	h->mode = mode;
	return (struct __syscall_ret){.ret = fd, .errno = 0};
}
struct __syscall_ret syscall_chdir(const char *path) {
	struct process_t *proc = get_process_start();
	struct vnode *cwd = proc->cwd;

	char *oldpath = proc->cwdpath;
	get_process_finish(proc);
	char *newpath = strdup(path);
	sprintf("syscall_chdir(): switching from old path %s to new path %s\r\n",
			oldpath, newpath);
	if (newpath[0] == '/') {
		struct vnode *out = NULL;
		int res = vfs_lookup(NULL, newpath, &out);
		if (res != 0 || out == NULL) {
			return (struct __syscall_ret){.ret = -1, .errno = ENOENT};
		} else if (out->stat.st_mode & ~S_IFDIR) {
			return (struct __syscall_ret){.ret = -1, .errno = ENOTDIR};
		}
		proc = get_process_start();
		proc->cwd = out;
		proc->cwdpath = newpath;
		get_process_finish(proc);
		return (struct __syscall_ret){.ret = 0, .errno = 0};
	} else {
		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
}
struct __syscall_ret syscall_poll(struct pollfd *fds, nfds_t nfds,
								  int timeout) {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();

	sprintf(
		"poll(): fuck you! number of fds: %lu, events: %d, targetted fd: %d, "
		"timeout: %d\r\n",
		nfds, fds->events, fds->fd, timeout);
	if (timeout != -1) {

		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
	if (nfds > 1) {

		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
	if (!fds) {

		return (struct __syscall_ret){.ret = -1, .errno = EINVAL};
	}
	struct FileDescriptorHandle *hnd = get_fd(fds->fd);
	int ret = hnd->node->ops->poll(hnd->node, fds);
	if (ret != 0) {

		return (struct __syscall_ret){.ret = -1, .errno = ret};
	}

	return (struct __syscall_ret){.ret = 1, .errno = 0};
}
struct __syscall_ret syscall_readdir(int fd, void *buf, size_t size) {
	size = ROUND_DOWN(size, sizeof(struct linux_dirent64));
	struct FileDescriptorHandle *hnd = get_fd(fd);
	sprintf("KANKER_readdir(): %d\r\n", fd);
	if (hnd == NULL) {

		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node == NULL) {

		return (struct __syscall_ret){.ret = -1, .errno = EIO};
	}
	// cache em
	struct dirstream *star = NULL;
	if (hnd->offset == 0) {
		int res = 0;
		star = hnd->node->ops->getdirents(hnd->node, &res);
		spinlock_lock(&star->write_lock);
		hnd->privatedata = star;
		spinlock_unlock(&star->write_lock);
		if (res != 0) {
			return (struct __syscall_ret){.ret = -1, .errno = res};
		}
	} else {
		star = hnd->privatedata;
	}

	// this is okay to do as no 1 FUCK YOU IM MEMCPYING TO USR SPACE
	// 2 entries are at least ensured
	size_t test = hnd->offset / sizeof(struct linux_dirent64);
	size_t new_idx = size / sizeof(struct linux_dirent64);
	if (star->position != test) {
		star->position = test;
	}
	if (star->position == star->cnt) {
		return (struct __syscall_ret){.ret = 0, .errno = 0};
	}
	bool cry = false;
	if (star->position + new_idx >= star->cnt) {
		size = sizeof(struct linux_dirent64);
		cry = true;
	}
	memcpy(buf, star->list + star->position, size);
	if (!cry) {
		hnd->offset += size;
		star->position += new_idx;
	} else {
		size = sizeof(struct linux_dirent64);
		star->position = star->cnt;
		hnd->offset = star->cnt * sizeof(struct linux_dirent64);
	}
	return (struct __syscall_ret){.ret = size, .errno = 0};
}
struct __syscall_ret syscall_read(int fd, void *buf, size_t count) {
	//sprintf("syscall_read(): fd %d, buf %p, size %lu\r\n", fd, buf, count);
	struct per_cpu_data *cpu = arch_get_per_cpu_data();

	struct FileDescriptorHandle *hnd = get_fd(fd);
	sprintf("KANKER_read(): %d\r\n", fd);
	// sprintf("syscall_read(): reading fd %d, has flags %d\r\n", fd,
	// hnd->flags);
	if (hnd == NULL) {

		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node == NULL) {

		return (struct __syscall_ret){.ret = -1, .errno = EIO};
	}
	if (hnd->node->v_type == VFIFO) {
		int res = 0;
		size_t trl = fifo_read((struct fifo *)hnd->node->data, buf, count, &res,
							   hnd->flags & O_NONBLOCK ? false : true);
		if (res != 0) {
			return (struct __syscall_ret){.ret = -1, .errno = res};
		}
		sprintf("syscall_read(): fifo read of amount %lu\r\n", trl);
		return (struct __syscall_ret){.ret = trl, .errno = 0};
	}
	if (count > (hnd->node->stat.size - hnd->offset) &&
		hnd->node->stat.size != 0 && hnd->node->v_type != VCHRDEVICE) {
		count = hnd->node->stat.size - hnd->offset;
	}
	int res = 0;
	size_t bytes_read =
		hnd->node->ops->rw(hnd->node, hnd->offset, count, buf, 0, hnd, &res);
	if (res != 0) {
		
		return (struct __syscall_ret){.ret = -1, .errno = res};
	}
	hnd->offset += bytes_read;
	sprintf("syscall_read(): read %lu\r\n", bytes_read);
	return (struct __syscall_ret){.ret = bytes_read, .errno = 0};
}
struct __syscall_ret syscall_close(int fd) {
	sprintf("syscall_close(): closing fd %d\r\n", fd);
	struct FileDescriptorHandle *hnd = get_fd(fd);
	if (hnd == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node->v_type == VFIFO) {
		// sure we def did
		fifo_close(hnd);
		return (struct __syscall_ret){.ret = 0, .errno = 0};
	}
	int ret = hnd->node->ops->close(hnd->node, hnd);
	
	if (ret != 0) {

		return (struct __syscall_ret){.ret = -1, .errno = ret};
	}
	fddfree(fd);
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_seek(int fd, long int long offset, int whence) {
	struct FileDescriptorHandle *hnd = get_fd(fd);
	sprintf("KANKER_seek(): %d\r\n", fd);
	if (hnd == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node->v_type == VFIFO) {
		return (struct __syscall_ret){.ret = -1, .errno = ESPIPE};
	}
	switch (whence) {
	case SEEK_SET:
		hnd->offset = offset;
		if (offset == 0 && hnd->flags & O_DIRECTORY) {
			struct dirstream *star = hnd->privatedata;
			spinlock_lock(&star->write_lock);
			kfree(star->list, (star->cnt + 1) * sizeof(struct linux_dirent64));
			kfree(hnd->privatedata, sizeof(struct dirstream));
			spinlock_unlock(&star->write_lock);
		}
		break;
	case SEEK_CUR:
		hnd->offset += offset;

		break;
	case SEEK_END:
		hnd->offset = hnd->node->stat.size + offset;
		if (hnd->flags & O_DIRECTORY) {
			panic(">:3 no");
		}
		break;
	default:
		return (struct __syscall_ret){.ret = -1, .errno = EINVAL};
	}
	return (struct __syscall_ret){.ret = hnd->offset, .errno = 0};
}
struct __syscall_ret syscall_isatty(int fd) {

	struct FileDescriptorHandle *hnd = get_fd(fd);
	sprintf("syscall_isatty(): call on fd %d\r\n", fd);
	if (hnd == NULL) {
		sprintf("syscall_isatty(): fd %d not a tty\r\n", fd);
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node->v_type == VCHRDEVICE) {
		struct devfsnode *nod = hnd->node->data;
		if (nod->info->major == 4) {
			return (struct __syscall_ret){.ret = 0, .errno = 0};
		}
	}
	return (struct __syscall_ret){.ret = 0, .errno = ENOTTY};
}
struct __syscall_ret syscall_write(int fd, const void *buf, size_t count) {
	//sprintf("syscall_write(): fd %d, buf %p, size %lu\r\n", fd, buf, count);
	struct FileDescriptorHandle *hnd = get_fd(fd);
	if (hnd == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	if (hnd->node->v_type == VFIFO) {
		int res = 0;
		size_t rett =
			fifo_write((struct fifo *)hnd->node->data, (uint8_t *)buf, count,
					   &res, hnd->flags & O_NONBLOCK ? false : true);
		sprintf("fifo write of nonblock ? %d, bytes written %lu, count wanted %lu\r\n", hnd->flags & O_NONBLOCK, rett, count);
		if (res != 0) {
			sprintf("fifo error of %d\r\n", res);
			return (struct __syscall_ret){.ret = -1, .errno = res};
		}
		return (struct __syscall_ret){.ret = rett, .errno = 0};
	}
	int res = 0;
	size_t written = hnd->node->ops->rw(hnd->node, hnd->offset, count,
										(void *)buf, 1, hnd, &res);
	if (res) {
		return (struct __syscall_ret){.ret = -1, .errno = res};
	}
	return (struct __syscall_ret){.ret = written, .errno = 0};
}
struct __syscall_ret syscall_ioctl(int fd, unsigned long request, void *arg) {
	struct FileDescriptorHandle *hnd = get_fd(fd);
	if (hnd == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	};
	sprintf("syscall_ioctl(): ioctling fd %d\r\n", fd);
	void *result;
	int res = hnd->node->ops->ioctl(hnd->node, request, arg, &result);
	sprintf("done\r\n");
	return (struct __syscall_ret){.ret = (uint64_t)result, .errno = res};
}
struct __syscall_ret syscall_dup(int fd, int flags) {

	struct FileDescriptorHandle *hnd = get_fd(fd);
	if (hnd == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	int newfd = fddup(fd);
	sprintf("syscall_dup(): duping fd %d to fd %d and flags %d\r\n", fd, newfd,
			flags);

	return (struct __syscall_ret){.ret = (uint64_t)newfd, .errno = 0};
}
struct __syscall_ret syscall_dup2(int oldfd, int newfd) {
	sprintf("syscall_dup2(): oldfd %d, newfd %d\r\n", oldfd, newfd);
	struct FileDescriptorHandle *check = get_fd(oldfd);
	if (check == NULL || check->node == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}
	struct FileDescriptorHandle *hnd = get_fd(newfd);

	if (hnd != NULL) {
		syscall_close(newfd); // do it sliently as per man page
	}
	fdmake(oldfd, newfd);
	return (struct __syscall_ret){.ret = (uint64_t)newfd, .errno = 0};
}
struct __syscall_ret syscall_fstat(int fd, struct stat *output, int flags,
								   char *path) {
	sprintf("syscall_fstat(): trying to access fd %d, flags %x\r\n", fd, flags);
	if (fd == AT_FDCWD) {
		sprintf("path %s\r\n", path);
		if (path[0] == '.' && path[1] == '\0') {
			struct process_t *proc = get_process_start();
			struct vnode *cwd = proc->cwd;
			*output = cwd->stat;
			get_process_finish(proc);
			sprintf("syscall_fstat(): output address %p, size %lu, mode %x\r\n",
					output, output->size, output->st_mode);
			return (struct __syscall_ret){.ret = 0, .errno = 0};
		}
		struct process_t *proc = get_process_start();
		struct vnode *cwd = proc->cwd;
		struct vnode *node = NULL;
		int res = vfs_lookup(cwd, path, &node);
		get_process_finish(proc);
		if (node == NULL || res != 0) {
			return (struct __syscall_ret){.ret = -1, .errno = res};
		}
		if (flags & AT_EMPTY_PATH) {
			sprintf("syscall_fstat(): empty path\r\n");
			*output = cwd->stat;
		} else
			*output = node->stat;
		sprintf("syscall_fstat(): output address %p, size %lu, mode %x\r\n",
				output, output->size, output->st_mode);
		return (struct __syscall_ret){.ret = 0, .errno = 0};
	}

	struct FileDescriptorHandle *hnd = get_fd(fd);
	if (hnd == NULL) {
		sprintf("syscall_fstat(): bad file descriptor\r\n");
		return (struct __syscall_ret){.ret = -1, .errno = EBADF};
	}

	if (path == NULL) {
		sprintf("syscall_fstat(): flags %d\r\n", flags);

		goto b;
	}
	sprintf("syscall_fstat(): flags %d, path %s\r\n", flags, path);
	if (path[0] != '\0') {
		if (path[0] != '/') {
			UNIMPL
		}
	}
b:
	if (flags != 0) {
		sprintf("sorry not impld'd\r\n");
		UNIMPL
	}
	*output = hnd->node->stat;

	sprintf("syscall_fstat(): output address %p, size %lu, mode %x\r\n", output,
			output->size, output->st_mode);
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_getcwd(char *buffer, size_t len) {
	struct process_t *proc = get_process_start();
	if (proc->cwdpath == NULL) {
		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
	// sprintf("syscall_getcwd(): size: %lu, len of buf %lu\r\n",
	//         strlen(proc->cwdpath), len);
	if (len < strlen(proc->cwdpath) + 1) {
		sprintf("\e[0;34mnope\r\n");
		get_process_finish(proc);
		return (struct __syscall_ret){.ret = -1, .errno = ERANGE};
	}
	memcpy(buffer, proc->cwdpath, len);
	get_process_finish(proc);
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_fork(int *pid) {
	sprintf("attempting fork\r\n");
	int child = scheduler_fork();
	sprintf("syscall_fork(): forked process to %d\r\n", child);
	*pid = child;
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_getpid() {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();

	sprintf("syscall_getpid(): giving pid %lu\r\n", cpu->cur_thread->proc->pid);
	return (struct __syscall_ret){.ret = cpu->cur_thread->proc->pid,
								  .errno = 0};
}
struct __syscall_ret syscall_waitpid(int pid, int *status, int flags) {

	// return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	struct per_cpu_data *cpu = arch_get_per_cpu_data();
	sprintf("syscall_waitpid(): wait on pid %d, flags %d\r\n", pid, flags);
	if (pid != -1) {
		sprintf("syscall_waitpid(): unsupported waitpit with pid %d\r\n", pid);
		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
neverstop:

	struct process_t *us = cpu->cur_thread->proc->children;
	if (!us) {
		return (struct __syscall_ret){.ret = -1, .errno = ECHILD};
	}
	//__asm__ volatile ("cli"); // we dont want the process to be unmapped by
	//the
	// reaper thread while we are doing this so
	// this is required
	while (us != NULL) {
		if (us->state == ZOMBIE) {
			sprintf("doing so with error code %lu\r\n", us->exit_code);
			*status = W_EXITCODE(us->exit_code, 0);
			us->cnt = 0;
			us->state = BLOCKED;
			uint64_t pidnum = us->pid;
			return (struct __syscall_ret){.ret = pidnum, .errno = 0};
		}
		us = us->children_next;
	}
	// __asm__ volatile ("sti"); // restore interrupts
	sched_yield();

	goto neverstop;
}
static void read_shit(void *data, void *variable) {
	*(__int128_t *)variable = info.timestamp;
}
struct __syscall_ret syscall_clockget(int clock, long *time, long *nanosecs) {
	switch (clock) {
	case CLOCK_REALTIME:
		__int128_t timestamp = 0;
		seq_read(&info.lock, read_shit, NULL, &timestamp);
		timestamp = timestamp + GenericTimerGetns();
		if (timestamp < 0) {
			__int128_t seconds = (timestamp - (1000000000 - 1)) / 1000000000;
			*time = seconds;
			*nanosecs = timestamp - (seconds * 1000000000);

			return (struct __syscall_ret){.ret = 0, .errno = 0};
		} else {
			*time = timestamp / 1000000000;
			*nanosecs = timestamp % 1000000000;

			return (struct __syscall_ret){.ret = 0, .errno = 0};
		}

		break;
	default:
		break;
	}
	return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
}
struct __syscall_ret syscall_faccessat(int dirfd, const char *pathname,
									   int mode, int flags) {
	sprintf("syscall_faccessat\r\n");
	if (flags & AT_SYMLINK_NOFOLLOW) {
		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
	if (dirfd != AT_FDCWD) {
		return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
	}
	struct vnode *node = NULL;
	struct process_t *proc = get_process_start();
	node = proc->cwd;
	get_process_finish(proc);
	struct vnode *retur = NULL;
	int res = vfs_lookup(node, pathname, &retur);
	if (res != 0) {
		return (struct __syscall_ret){.ret = -1, .errno = ENOENT};
	}
	sprintf("syscall_faccessat(): ignoring mode %d flags %d\r\n", mode, flags);
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_pipe(int outputs[2], int flags) {
	struct process_t *proc = get_process_start();
	if (outputs == NULL && arch_is_mapped_buf(proc->cur_map, (uint64_t)outputs,
											  sizeof(int[2])) == false) {
		get_process_finish(proc);
		return (struct __syscall_ret){.ret = -1, .errno = EINVAL};
	}
	get_process_finish(proc);
	pipe_create(outputs);
	struct FileDescriptorHandle *read = get_fd(outputs[0]);
	struct FileDescriptorHandle *write = get_fd(outputs[1]);
	read->flags = flags;
	write->flags = flags;
	return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_execve(const char *path, char *const argv[],
									char *const envp[]) {
	struct per_cpu_data *cpu = arch_get_per_cpu_data();
	struct vnode *cwd = NULL;
	struct process_t *proc = get_process_start();
	cwd = proc->cwd;
	get_process_finish(proc);
	struct vnode *result;
	size_t iter = 0;
	void *item;
	while (hashmap_iter(cpu->cur_thread->proc->fds, &iter, &item)) {
		struct FileDescriptorHandle *hnd =
				(struct FileDescriptorHandle *)item;
			if (hnd->flags & O_CLOEXEC) {
				if (hnd->node->v_type == VFIFO) {
					// fifo close would be called
					continue;
				}
				hnd->node->ops->close(hnd->node, hnd);
			}
		}
	sprintf("syscall_execve(): loading elf %s\r\n", path);
	char *kernelpath = strdup(path);
	int res = vfs_lookup(cpu->cur_thread->proc->cwd, path, &result);
	if (res != 0) {
		return (struct __syscall_ret){.ret = -1, .errno = ENOENT};
	} else {
		int argc = 0;
		while (argv[argc] != NULL) {
			argc++;
		}
		int g = 0;
		while (envp[g] != NULL) {
			g++;
		}

		char **newargv = kmalloc((argc + 1) * 8);
		char **newenvp = kmalloc((g + 1) * 8);
		newargv[argc] = NULL;
		newenvp[g] = NULL;
		for (int i = 0; i < argc; i++) {
			char *blah = strdup(argv[i]);
			newargv[i] = blah;
		}
		for (int i = 0; i < g; i++) {
			char *blah = strdup(envp[i]);
			newenvp[i] = blah;
		}
		deallocate_all_user_regions(cpu->cur_thread->proc->cur_map);
		clear_and_prepare_thread(cpu->cur_thread);
		sprintf("syscall_execve(): Ready To Execute\r\n");
		load_elf(cpu->cur_thread->proc->cur_map, kernelpath, newargv, newenvp,
				 &cpu->cur_thread->arch_data.frame, cwd);
		kfree(newenvp, (g + 1) * 8);
		kfree(newargv, (argc + 1) * 8);

		schedd(NULL);
		panic("hello?\r\n");
		// is anyone there?
		// its dark..
		// its so dark..
		// can anyone hear me?
		// anyone?...
	}
}
extern void syscall_entry();

void syscall_init() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
	if (edx & (1 << 11)) {
		uint64_t IA_32_STAR = 0;
		IA_32_STAR |= ((uint64_t)0x28 << 32);
		IA_32_STAR |= ((uint64_t)0x33 << 48);
		wrmsr(0xC0000081, IA_32_STAR);
		wrmsr(0xC0000082, (uint64_t)syscall_entry);
		wrmsr(0xC0000084, (1 << 9));
		uint64_t IA_32_EFER = rdmsr(0xC0000080);
		IA_32_EFER |= (1);
		wrmsr(0xC0000080, IA_32_EFER);

	} else {
		panic(
			"the syscall instruction is NOT supported. nyaux needs it to run");
	}
}
