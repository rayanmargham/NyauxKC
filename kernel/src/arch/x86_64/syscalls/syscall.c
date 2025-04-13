#include "syscall.h"
#include "../instructions/instructions.h"
#include "elf/symbols/symbols.h"
#include "fs/devfs/devfs.h"
#include "fs/tmpfs/tmpfs.h"
#include "fs/vfs/fd.h"
#include "fs/vfs/vfs.h"
#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"

#include <stdint.h>

struct __syscall_ret syscall_exit(int exit_code) {
  sprintf("syscall_exit(): exit_code %d\r\n", exit_code);
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  if (cpu->cur_thread->proc->pid == 0) {
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
  sprintf("syscall_mmap(): size %lu flags %x\n\n", size, flags);
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  if (flags & MAP_ANONYMOUS) {
    if (hint != 0) {
      return (struct __syscall_ret){
          (uint64_t)uvmm_region_alloc_fixed(cpu->cur_thread->proc->cur_map,
                                            (uint64_t)hint, size, false),
          0};
    }

    return (struct __syscall_ret){(uint64_t)uvmm_region_alloc_demend_paged(
                                      cpu->cur_thread->proc->cur_map, size),
                                  0};
  }
  return (struct __syscall_ret){-1, ENOSYS};
}
struct __syscall_ret syscall_free(void *pointer, size_t size) {
  sprintf("syscall_free(): freeing %lu\r\n", size);
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  uvmm_region_dealloc(cpu->cur_thread->proc->cur_map, pointer);
  return (struct __syscall_ret){-0, 0};
}
struct __syscall_ret syscall_openat(int dirfd, const char *path, int flags,
                                    unsigned int mode) {
  sprintf("syscall_openat(): opening %s from thread %lu with flags %d\r\n",
          path, arch_get_per_cpu_data()->cur_thread->tid, flags);
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
  int outfd = fddalloc(retur);
  struct FileDescriptorHandle *setup = get_fd(outfd);
  setup->flags = flags;
  setup->mode = mode;
  return (struct __syscall_ret){.ret = outfd, .errno = 0};
}
struct __syscall_ret syscall_read(int fd, void *buf, size_t count) {
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  if (hnd->node == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EIO};
  }
  if (count > (hnd->node->stat.size - hnd->offset) &&
      hnd->node->stat.size != 0 && hnd->node->v_type != VCHRDEVICE) {
    count = hnd->node->stat.size - hnd->offset;
  }
  size_t bytes_read =
      hnd->node->ops->rw(hnd->node, hnd->offset, count, buf, 0, hnd);
  hnd->offset += bytes_read;
  return (struct __syscall_ret){.ret = bytes_read, .errno = 0};
}
struct __syscall_ret syscall_close(int fd) {
  sprintf("syscall_close()\r\n");
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  fddfree(fd);
  return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_seek(int fd, int64_t offset, int whence) {
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
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }

  size_t written =
      hnd->node->ops->rw(hnd->node, hnd->offset, count, (void *)buf, 1, hnd);
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
  sprintf("syscall_dup2\r\n");
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
struct __syscall_ret syscall_fstat(int fd, struct stat *output) {
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    sprintf("syscall_fstat(): bad file descriptor\r\n");
    return (struct __syscall_ret){.ret = -1, .errno = EBADF};
  }
  *output = hnd->node->stat;
  sprintf("syscall_fstat(): output address %p, size %lu, mode %x\r\n", output,
          output->size, output->st_mode);
  return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_getcwd(char *buffer, size_t len) {
  struct process_t *proc = get_process_start();
  sprintf("syscall_getcwd(): size: %lu, len of buf %lu\r\n",
          strlen(proc->cwdpath), len);
  if (len < strlen(proc->cwdpath) + 1) {
    sprintf("\e[0;34mnope\r\n");
    get_process_finish(proc);
    return (struct __syscall_ret){.ret = -1, .errno = ERANGE};
  }
  memcpy(buffer, proc->cwdpath, len);
  get_process_finish(proc);
  return (struct __syscall_ret){.ret = 0, .errno = 0};
}
struct __syscall_ret syscall_fork() {
  int child = scheduler_fork();
  sprintf("syscall_fork(): forked process to %d\r\n", child);
  return (struct __syscall_ret){.ret = child, .errno = 0};
}
struct __syscall_ret syscall_getpid() {
  sprintf("syscall_getpid(): getting pid\r\n");
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  return (struct __syscall_ret){.ret = cpu->cur_thread->proc->pid, .errno = 0};
}
struct __syscall_ret syscall_waitpid(int pid, int *status, int flags) {

  // return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  sprintf("syscall_waitpid(): wait on pid %d, flags %d\r\n", pid, flags);
  if (pid != -1) {
    return (struct __syscall_ret){.ret = -1, .errno = ENOSYS};
  }
neverstop:

  cpu->cur_thread->syscall_user_sp = cpu->arch_data.syscall_stack_ptr_tmp;
  struct process_t *us = cpu->cur_thread->proc->children;
  if (!us) {
    return (struct __syscall_ret){.ret = -1, .errno = ECHILD};
  }
  while (us != NULL) {

    if (us->state == ZOMBIE) {
      sprintf("doing so with error code %lu\r\n", us->exit_code);
      *status = W_EXITCODE(us->exit_code, 0);
      us->cnt = 0;
      us->state = BLOCKED;
      return (struct __syscall_ret){.ret = us->pid, .errno = 0};
    }
    if (us->children == us) {
      break;
    }
    us = us->children;
  }
  sched_yield();

  cpu->arch_data.syscall_stack_ptr_tmp = cpu->cur_thread->syscall_user_sp;
  goto neverstop;
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
struct __syscall_ret syscall_execve(const char *path, char *const argv[],
                                    char *const envp[]) {
  struct per_cpu_data *cpu = arch_get_per_cpu_data();
  struct vnode *result;
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
             &cpu->cur_thread->arch_data.frame);
    sprintf("okay\r\n");
    schedd(NULL);
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