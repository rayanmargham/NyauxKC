#pragma once

#include "fs/vfs/fd.h"
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
#ifdef __cplusplus
extern "C" {
#endif
void vfs_init();
enum vtype {
  VREG,
  VFIFO,
  VCHRDEVICE,
  VBLKDEVICE,
  VDIR,
  VSYMLINK // symlink
};
struct pollfd {
  int fd;        /* file descriptor*/
  short events;  /* requested events */
  short revents; /* returned events */
};
typedef unsigned long int nfds_t;
#define POLLIN 0x001   /* There is data to read.  */
#define POLLPRI 0x002  /* There is urgent data to read.  */
#define POLLOUT 0x004  /* Writing now will not block.  */
#define POLLERR 0x008  /* Error condition.  */
#define POLLHUP 0x010  /* Hung up.  */
#define POLLNVAL 0x020 /* Invalid polling request.  */
extern struct vfs *vfs_list;
struct timespec {

  long tv_sec;  /* Seconds.  */
  long tv_nsec; /* Nanoseconds.  */
};
#define S_IFMT 0x0F000   /* bit mask for the file type bit field*/
#define S_IFBLK 0x06000  /* block device*/
#define S_IFCHR 0x02000  /* character device*/
#define S_IFIFO 0x01000  /* fifo*/
#define S_IFREG 0x08000  /* regular file*/
#define S_IFDIR 0x04000  /* directory */
#define S_IFLNK 0x0A000  /* sym link*/
#define S_IFSOCK 0x0C000 /* socket*/
struct stat {
  unsigned long st_dev;
  unsigned long st_ino;
  unsigned long st_nlink;
  unsigned int st_mode;
  unsigned int st_uid;
  unsigned int st_gid;
  unsigned int __pad0;
  unsigned long st_rdev;
  long size;
  long st_blksize;
  long st_blocks;
  struct timespec st_atim;
  struct timespec st_mtim;
  struct timespec st_ctim;
  long __unused[3];
};
struct vnode {
  struct vfs *vfs;
  struct vnodeops *ops;
  enum vtype v_type;
  struct stat stat;
  void *data;
};
struct vnodeops {
  int (*close)(struct vnode *curvnode, int fd);
  int (*open)(struct vnode *curvnode, int flags, unsigned int mode, int *res);
  int (*lookup)(struct vnode *curvnode, char *name, struct vnode **res);
  int (*create)(struct vnode *curvnode, char *name, enum vtype type,
                struct vnodeops *ops, struct vnode **res, void *data,
                struct vnode *todifferentnode);
  int (*poll)(struct vnode *curvnode, struct pollfd *requested);
  // curvnode, offset, size, buffer, rw
  size_t (*rw)(struct vnode *curvnode, size_t offset, size_t size, void *buffer,
               int rw, struct FileDescriptorHandle *hnd, int *res);
  struct dirstream *(*getdirents)(struct vnode *curvnode, int *res);
  int (*ioctl)(struct vnode *curvnode, unsigned long request, void *arg,
               void *result);
  // will throw vnode into dir with the name
  int (*hardlink)(struct vnode *curvnode, struct vnode *with, const char *name);
};
struct vfs_ops {
  int (*mount)(struct vfs *curvfs, char *path, void *data);
};
struct vfs {
  struct vfs *vfs_next;
  struct vfs_ops *vfs_ops;
  struct vnode *cur_vnode;
};
void vfs_create_from_tar(char *path, enum vtype type, size_t filesize,
                         void *buf);
void vfs_scan();
int vfs_lookup(struct vnode *start, const char *path, struct vnode **node);
#define O_PATH 010000000
struct linux_dirent64 {
	uint64_t d_ino;
	int64_t d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		*d_name;
};
struct dirstream {
  uint64_t position;
  struct linux_dirent64 **list;
};
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

#define O_ACCMODE (03 | O_PATH)
#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02

#define O_CREAT 0100
#define O_EXCL 0200
#define O_NOCTTY 0400
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_DSYNC 010000
#define O_ASYNC 020000
#define O_DIRECT 040000
#define O_DIRECTORY 0200000
#define O_NOFOLLOW 0400000
#define O_CLOEXEC 02000000
#define O_SYNC 04010000
#define O_RSYNC 04010000
#define O_LARGEFILE 0100000
#define O_NOATIME 01000000
#define O_TMPFILE 020000000

#define O_EXEC O_PATH
#define O_SEARCH O_PATH

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4

#define F_SETOWN 8
#define F_GETOWN 9
#define F_SETSIG 10
#define F_GETSIG 11

#define F_GETLK 5
#define F_SETLK 6
#define F_SETLK64 F_SETLK
#define F_SETLKW 7
#define F_SETLKW64 F_SETLKW

#define F_SETOWN_EX 15
#define F_GETOWN_EX 16

#define F_GETOWNER_UIDS 17

#define F_SETLEASE 1024
#define F_GETLEASE 1025
#define F_NOTIFY 1026
#define F_DUPFD_CLOEXEC 1030
#define F_SETPIPE_SZ 1031
#define F_GETPIPE_SZ 1032
#define F_ADD_SEALS 1033
#define F_GET_SEALS 1034

#define F_SEAL_SEAL 0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW 0x0004
#define F_SEAL_WRITE 0x0008

#define F_OFD_GETLK 36
#define F_OFD_SETLK 37
#define F_OFD_SETLKW 38

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2
#ifdef __cplusplus
}
#endif