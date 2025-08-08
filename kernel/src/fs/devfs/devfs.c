#include "devfs.h"

#include <dev/fbdev/fb.hpp>
#include <dev/keyboard/keyboard.hpp>
#include <dev/null/null.h>
#include <dev/tty/tty.h>
#include <fs/vfs/fd.h>
#include <fs/vfs/vfs.h>
#include <arch/x86_64/syscalls/syscall.h>
#include <fs/tmpfs/tmpfs.h>
#include <utils/libc.h>

static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data,
                  struct vnode *todifferentnode);
static int lookup(struct vnode *curvnode, char *name, struct vnode **res);
static size_t rww(struct vnode *curvnode, size_t offset, size_t size,
                  void *buffer, int rw, struct FileDescriptorHandle *hnd,
                  int *res);
static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result);

static int mount(struct vfs *curvfs, char *path, void *data);
static int poll(struct vnode *curvnode, struct pollfd *requested);
static int open(struct vnode *curvnode, int flags, unsigned int mode, int *res);
static int readdir(struct vnode *curvnode, struct FileDescriptorHandle *hnd, const char *name);
static int close (struct vnode *curvnode, int fd);
static int hardlink(struct vnode *curvnode, struct vnode *with,
                    const char *name);
static struct dirstream *getdirents(struct vnode *curvnode, int *res);
struct vnodeops vnode_devops = {.open = open,
  .close = close,
  .lookup = lookup,
                                .create = create,
                                .rw = rww,
                                .getdirents = getdirents,
                                .ioctl = ioctl,
                                .poll = poll,
                                .hardlink = hardlink};
struct vfs_ops vfs_devops = {.mount = mount};
static int mount(struct vfs *curvfs, char *path, void *data) {
  devnull_init(curvfs);

  devfbdev_init(curvfs);
  devkbd_init(curvfs);
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
  struct devfsdirentry *direntry = node->direntry;
  struct devfsnode *dot = (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
  dot->name = ".";
  dot->curvnode = replacement;
  dot->direntry = direntry;
  struct devfsnode *dotdot =
      (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
  dotdot->name = "..";

  replacement->ops = &vnode_devops;
  replacement->v_type = VDIR;
  dotdot->curvnode = starter;
  dotdot->direntry =
      NULL; // this is fine as long as it is not accessed, which it will not be
  insert_into_list(dot, direntry);
  insert_into_list(dotdot, direntry);
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
                  void *buffer, int rw, struct FileDescriptorHandle *hnd,
                  int *res) {
  if (curvnode->v_type == VDIR) {
    *res = EISDIR;
    return 0;
  }
  struct devfsnode *devnode = (struct devfsnode *)curvnode->data;
  return devnode->info->ops->rw(curvnode, devnode->info->data, offset, size,
                                buffer, rw, hnd, res);
}
static int lookup(struct vnode *curvnode, char *name, struct vnode **res) {
  struct devfsnode *node = (struct devfsnode *)curvnode->data;
  if (node == NULL) {
    // todo: change this later
    panic("devfs(): node is null");
  }
  if (curvnode->v_type == VREG) {
    return -1;
  } else if (curvnode->v_type == VDIR) {
    struct devfsdirentry *entry = (struct devfsdirentry *)node->direntry;
    for (size_t i = 0; i < entry->cnt; i++) {
      sprintf("dev: found component \"%s\"\r\n", entry->nodes[i]->name);
      if (strcmp(entry->nodes[i]->name, name) == 0) {
        *res = entry->nodes[i]->curvnode;
        return 0;
      }
    }
    return -1;
  }
  return -1;
}

static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result) {
  struct devfsnode *devnode = (struct devfsnode *)curvnode->data;
  return devnode->info->ops->ioctl(curvnode, devnode->info->data, request, arg,
                                   result);
}
static int poll(struct vnode *curvnode, struct pollfd *requested) {
  struct devfsnode *devnode = (struct devfsnode *)curvnode->data;
  return devnode->info->ops->poll(curvnode, requested, devnode->info->data);
}
int hardlink(struct vnode *curvnode, struct vnode *with, const char *name) {
  if (curvnode->v_type != VDIR) {
    return EINVAL;
  }
  struct devfsnode *node = (struct devfsnode *)curvnode->data;
  assert(node);
  struct devfsdirentry *entry = (struct devfsdirentry *)node->direntry;
  struct devfsnode *new = (struct devfsnode *)kmalloc(sizeof(struct devfsnode));
  new->name = (char *)name;
  new->curvnode = with;
  insert_into_list(new, entry);
  return 0;
}
static int open(struct vnode *curvnode, int flags, unsigned int mode, int *res) {
 
  int fd = fddalloc(curvnode);
  struct FileDescriptorHandle *hnd = get_fd(fd);
  hnd->flags = flags;
  hnd->mode = mode;
  struct devfsnode *node = (struct devfsnode*)curvnode->data;
  if (node->direntry != NULL && node->name) {
    *res = EISDIR;
    return fd;
   }
  
  node->info->ops->open(curvnode, node->info->data, res, hnd);
  return fd;
}
static int close (struct vnode *curvnode, int fd) {
  struct FileDescriptorHandle *hnd = get_fd(fd);
  if (hnd == NULL) {
    return EBADF;
  }
  struct devfsnode *node = (struct devfsnode*)curvnode->data;
  if (node->info == NULL) {
    fddfree(fd);
    return 0;
  } 
  int res = node->info->ops->close(curvnode, node->info->data, hnd);
  fddfree(fd);
  return res;
}

static struct dirstream *getdirents(struct vnode *curvnode, int *res) {

}