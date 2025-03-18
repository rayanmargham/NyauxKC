#include "syscall.h"
#include "../instructions/instructions.h"
#include "elf/symbols/symbols.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"

#include <stdint.h>

struct __syscall_ret syscall_exit(int exit_code) {
  kprintf("syscall_exit(): exit_code %d\r\n", exit_code);
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  struct process_t *proc = get_process_start();
  proc->exit_code = exit_code;
  get_process_finish(proc);
  cpu->cur_thread->state = ZOMBIE;
  // kill all threads TODO
  sched_yield();
  panic("shouldn't be here");
  // return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_debug(char *string, size_t length) {
  char *buffer = kmalloc(1024);
  memcpy(buffer, string, length);
  buffer[length] = '\0';
  kprintf("userland: %s\r\n", buffer);
  return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_setfsbase(uint64_t ptr) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  cpu->cur_thread->arch_data.fs_base = ptr;
  wrmsr(0xC0000100, cpu->cur_thread->arch_data.fs_base);
  return (struct __syscall_ret){0, 0};
}
struct __syscall_ret syscall_mmap(void *hint, size_t size, int prot, int flags,
                                  int fd, size_t offset) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  if (flags & MAP_ANONYMOUS) {
    if (hint != 0) {
      return (struct __syscall_ret){
          (uint64_t)uvmm_region_alloc_fixed(cpu->cur_thread->proc->cur_map,
                                            (uint64_t)hint, size, false),
          0};
    }
    return (struct __syscall_ret){
        (uint64_t)uvmm_region_alloc(cpu->cur_thread->proc->cur_map, size, 0),
        0};
  }
  return (struct __syscall_ret){-1, ENOSYS};
}
struct __syscall_ret syscall_openat(int dirfd, const char *path, int flags,
                                    unsigned int mode) {
  kprintf("sys_openat(): opening path %s\r\n", path);
  struct vnode *node = NULL;
  if (dirfd == -100) {
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
  int outfd = fddalloc(retur);
  return (struct __syscall_ret){.ret = outfd, .errno = 0};
}
struct __syscall_ret syscall_read(int fd, void *buf, size_t count) {
  kprintf("sys_read(): reading fd %d\r\n", fd);
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  if (hnd->node == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EIO};
  }
  size_t bytes_written =
      hnd->node->ops->rw(hnd->node, hnd->offset, count, buf, 0);
  hnd->offset += bytes_written;
  return (struct __syscall_ret){.ret = bytes_written, .errno = 0};
}
struct __syscall_ret syscall_close(int fd) {
  kprintf("sys_close(): closing fd %d\r\n", fd);
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  fddfree(fd);
  return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_seek(int fd, int64_t offset, int whence) {
  kprintf("sys_seek(): seeking with fd %d\r\n", fd);
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  switch (whence) {
  case SEEK_SET:
    hnd->offset = offset;
    break;
  case SEEK_CUR:
    hnd->offset += offset;
    break;
  case SEEK_END:
    hnd->offset = hnd->node->stat.size + offset;
    break;
  default:
    return (struct __syscall_ret){.ret = -1, .errno = EINVAL};
  }
  return (struct __syscall_ret){.ret = hnd->offset, .errno = 0};
}
struct __syscall_ret syscall_isatty(int fd) {
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
}
extern void syscall_entry();

void syscall_init() {
  uint32_t eax, ebx, ecx, edx;
  cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
  if (edx & (1 << 11)) {
    uint64_t IA_32_STAR = 0;
    IA_32_STAR |= ((uint64_t)0x28 << 32);
    IA_32_STAR |= ((uint64_t)0x30 << 48);
    wrmsr(0xC0000081, IA_32_STAR);
    wrmsr(0xC0000082, (uint64_t)syscall_entry);
    wrmsr(0xC0000084, (1 << 9));
    uint64_t IA_32_EFER = rdmsr(0xC0000080);
    IA_32_EFER |= (1);
    wrmsr(0xC0000080, IA_32_EFER);

  } else {
    panic("the syscall instruction is NOT supported. nyaux needs it to run");
  }
}