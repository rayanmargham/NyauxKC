#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
void vfs_init();
enum vtype {
  VREG,
  VDEVICE,
  VDIR,
  VSYMLINK // symlink
};
extern struct vfs *vfs_list;
struct stat {
  size_t size;
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
