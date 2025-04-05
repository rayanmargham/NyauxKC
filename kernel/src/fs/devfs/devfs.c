#include "devfs.h"

#include <fs/tmpfs/tmpfs.h>
#include <utils/libc.h>

#include "dev/null/null.h"
#include "dev/tty/tty.h"
#include "fs/vfs/vfs.h"

static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data,
                  struct vnode *todifferentnode);
static int lookup(struct vnode *curvnode, char *name, struct vnode **res);
static size_t rww(struct vnode *curvnode, size_t offset, size_t size,
                  void *buffer, int rw);
static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result);
static int readdir(struct vnode *curvnode, int offset, char **name);
static int mount(struct vfs *curvfs, char *path, void *data);
struct vnodeops vnode_devops = {
    .lookup = lookup, .create = create, .rw = rww, readdir, .ioctl = ioctl};
struct vfs_ops vfs_devops = {.mount = mount};
static int mount(struct vfs *curvfs, char *path, void *data) {
  devnull_init(curvfs);
  devtty_init(curvfs);
  return 0;
}
static void insert_into_list(struct devfsnode *node,
                             struct devfsdirentry *entry) {
  if (entry->cnt == 0) {
    entry->nodes = kmalloc(sizeof(struct devfsnode *));
    entry->nodes[0] = node;
  } else {
    struct devfsnode **oldnodes = entry->nodes;
    entry->nodes = kmalloc(sizeof(struct devfsnode *) * (entry->cnt + 1));
    memcpy(entry->nodes, oldnodes, entry->cnt * sizeof(struct devfsnode *));
    kfree(oldnodes, entry->cnt * sizeof(struct devfsnode *));
    entry->nodes[entry->cnt] = node;
  }
  entry->cnt += 1;
}
void devfs_init(struct vfs *curvfs) {
  struct vnode *starter = NULL;
  vfs_lookup(NULL, "/", &starter);
  kprintf("devfs(): init\r\n");
  struct vfs *new = kmalloc(sizeof(struct vfs));
  new->vfs_ops = &vfs_devops;
  struct vnode *replacement = kmalloc(sizeof(struct vnode));
  replacement->data = kmalloc(sizeof(struct devfsnode));
  struct devfsnode *node = replacement->data;
  node->name = "dev";
  node->direntry = kmalloc(sizeof(struct devfsdirentry));
  replacement->ops = &vnode_devops;
  replacement->v_type = VDIR;
  starter->ops->create(starter, "dev", VDIR, &vnode_devops, &replacement, NULL,
                       replacement);
  struct vfs *old = curvfs;
  while (true) {
    if (old->vfs_next == NULL) {
      break;
    }
    old = old->vfs_next;
  }
  old->vfs_next = new;
  new->cur_vnode = replacement;
  new->vfs_ops->mount(new, "/", NULL);
}

static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data,
                  struct vnode *todifferentnode) {
  if (curvnode->v_type == VDIR) {
    struct devfsnode *node = (struct devfsnode *)curvnode->data;
    struct devfsdirentry *entry = (struct devfsdirentry *)node->direntry;
    if (type == VDIR) {
      struct devfsnode *dir =
          (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
      struct devfsdirentry *direntry =
          (struct devfsdirentry *)kmalloc(sizeof(struct devfsdirentry));
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
      newnode->data = dir;
      newnode->v_type = VDIR;
      newnode->ops = ops;
      newnode->vfs = curvnode->vfs;

      dir->direntry = direntry;
      direntry->cnt = 0;
      dir->name = name;
      dir->curvnode = newnode;
      dir->info = (struct devfsinfo *)data;

      struct devfsnode *dot =
          (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
      dot->name = ".";
      dot->curvnode = newnode;
      dot->direntry = direntry;
      struct devfsnode *dotdot =
          (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
      dotdot->name = "..";
      dotdot->curvnode = curvnode;
      dotdot->direntry = entry;
      insert_into_list(dot, direntry);
      insert_into_list(dotdot, direntry);
      insert_into_list(dir, entry);
      *res = newnode;
      return 0;
    } else if (type == VCHRDEVICE || type == VBLKDEVICE) {
      struct devfsnode *file =
          (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));

      newnode->v_type = type;
      newnode->ops = ops;
      newnode->vfs = curvnode->vfs;
      newnode->data = file;
      file->name = name;
      file->info = (struct devfsinfo *)data;
      newnode->stat.size = 0;
      insert_into_list(file, entry);
      file->curvnode = newnode;
      *res = newnode;
      return 0;
    }
  }
  kprintf("here error\r\n");
  return -1;
}
static size_t rww(struct vnode *curvnode, size_t offset, size_t size,
                  void *buffer, int rw) {
  struct devfsnode *devnode = (struct devfsnode *)curvnode->data;
  return devnode->info->ops->rw(curvnode, devnode->info->data, offset, size,
                                buffer, rw);
}
static int lookup(struct vnode *curvnode, char *name, struct vnode **res) {
  struct devfsnode *node = (struct devfsnode *)curvnode->data;
  if (node == NULL) {
    panic("devfs(): node is null");
  }
  if (curvnode->v_type == VREG) {
    return -1;
  } else if (curvnode->v_type == VDIR) {
    struct devfsdirentry *entry = (struct devfsdirentry *)node->direntry;
    for (size_t i = 0; i < entry->cnt; i++) {
      kprintf("address %p, %p, cnt %lu\r\n", entry->nodes[i], res, entry->cnt);
      if (strcmp(entry->nodes[i]->name, name) == 0) {
        kprintf("okay\r\n");
        *res = entry->nodes[i]->curvnode;
        return 0;
      }
    }
    return -1;
  }
  return -1;
}
static int readdir(struct vnode *curvnode, int offset, char **name) {
  return -1;
}
static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result) {
  struct devfsnode *devnode = (struct devfsnode *)curvnode->data;
  return devnode->info->ops->ioctl(curvnode, devnode->info->data, request, arg,
                                   result);
}