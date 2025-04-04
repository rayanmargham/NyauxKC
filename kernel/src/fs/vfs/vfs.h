#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
void vfs_init();
enum vtype {
  VREG,
  VCHRDEVICE,
  VBLKDEVICE,
  VDIR,
  VSYMLINK // symlink
};
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
  int (*lookup)(struct vnode *curvnode, char *name, struct vnode **res);
  int (*create)(struct vnode *curvnode, char *name, enum vtype type,
                struct vnodeops *ops, struct vnode **res, void *data,
                struct vnode *todifferentnode);
  // curvnode, offset, size, buffer, rw
  size_t (*rw)(struct vnode *curvnode, size_t offset, size_t size, void *buffer,
               int rw);
  int (*readdir)(struct vnode *curvnode, int offset, char **out);
  int (*ioctl)(struct vnode *curvnode, unsigned long request, void *arg,
               void *result);
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
