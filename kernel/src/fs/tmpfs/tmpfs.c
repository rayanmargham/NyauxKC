#include "tmpfs.h"
#include "fs/vfs/fd.h"
#include "utils/hashmap.h"

#include <arch/x86_64/syscalls/syscall.h>
#include <fs/vfs/vfs.h>
#include <mem/kmem.h>
#include <stddef.h>
uint64_t node_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct tmpfsnode *user = item;
  return hashmap_sip(user->name, strlen(user->name), seed0, seed1);
}
int node_compare(const void *a, const void *b, void *udata) {
  const struct tmpfsnode *ua = a;
  const struct tmpfsnode *ub = b;
  return strcmp(ua->name, ub->name);
}
static int open(struct vnode *cur_vnode, int flags, unsigned int mode, int *res);
static int close(struct vnode *curvnode, struct FileDescriptorHandle *hnd);
static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data,
                  struct vnode *todifferentnode);
static int lookup(struct vnode *curvnode, char *name, struct vnode **res);
static size_t rw(struct vnode *curvnode, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res);
static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result);
static int mount(struct vfs *curvfs, char *path, void *data);
static int poll(struct vnode *curvnode, struct pollfd *requested);
static int hardlink(struct vnode *curvnode, struct vnode *with,
                    const char *name);
static struct dirstream *getdirents(struct vnode *curvnode, int *res);
struct vnodeops tmpfs_ops = {
    .getdirents = getdirents, .close = close, .open = open, lookup, create, poll, rw, .ioctl = ioctl, .hardlink = hardlink};
struct vfs_ops tmpfs_vfsops = {mount};


static void insert_into_list(struct tmpfsnode *node, struct direntry *entry) {
  if (entry->cnt == 0) {
    entry->nodes = kmalloc(sizeof(struct tmpfsnode *));
    entry->nodes[0] = node;
  } else {
    struct tmpfsnode **oldnodes = entry->nodes;
    entry->nodes = kmalloc(sizeof(struct tmpfsnode *) * (entry->cnt + 1));
    memcpy(entry->nodes, oldnodes, entry->cnt * sizeof(struct tmpfsnode *));
    kfree(oldnodes, entry->cnt * sizeof(struct tmpfsnode *));
    entry->nodes[entry->cnt] = node;
  }
  entry->cnt += 1;
}
static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data,
                  struct vnode *todifferentnode) {
  if (curvnode->v_type == VDIR) {
    // if (strcmp(name, "dev") == 0) {
    //   panic("holy shit im doing this with tmpfs (which would make
    //   sense?)\r\n");
    // }
    struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
    struct direntry *entry = (struct direntry *)node->direntry;
    if (type == VDIR) {

      struct tmpfsnode *dir =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      if (!todifferentnode) {
        struct direntry *direntry =
            (struct direntry *)kmalloc(sizeof(struct direntry));
        dir->direntry = direntry;

        dir->name = name;
        dir->size = 0;
        struct tmpfsnode *dot =
            (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
        dot->name = ".";
        dot->size = 0;

        dot->direntry = direntry;

        struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
        newnode->stat.st_mode |= S_IFDIR;
        newnode->data = dir;
        newnode->v_type = VDIR;
        newnode->ops = ops;
        newnode->vfs = curvnode->vfs;
        dot->node = newnode;
        dir->node = newnode;
        struct tmpfsnode *dotdot =
            (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
        dotdot->name = "..";
        dotdot->size = 0;
        dotdot->node = curvnode;
        dotdot->direntry = entry;
        insert_into_list(dot, direntry);
        insert_into_list(dotdot, direntry);
        insert_into_list(dir, entry);
        *res = newnode;
        return 0;
      } else {
        dir->node = todifferentnode;
        dir->name = name;
        todifferentnode->stat.st_mode = S_IFDIR;
        sprintf("tmpfs(): different node data %p\r\n", todifferentnode->data);
        insert_into_list(dir, entry);
        *res = todifferentnode;
        return 0;
      }

      // kprintf("tmpfs(): created\r\n");
      return 0;
    } else if (type == VREG || type == VSYMLINK) {
      struct tmpfsnode *file =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));

      newnode->v_type = type;
      newnode->ops = ops;
      newnode->vfs = curvnode->vfs;
      newnode->data = file;
      file->name = name;
      file->size = 0;
      newnode->stat.size = 0;
      newnode->stat.st_mode = type == VREG ? S_IFREG : S_IFLNK;
      newnode->stat.st_mode |= O_RDWR;
      insert_into_list(file, entry);
      file->node = newnode;
      *res = newnode;
      return 0;
    }
  }
  kprintf("error\r\n");
  return -1;
}

static int lookup(struct vnode *curvnode, char *name, struct vnode **res) {
  struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
  if (curvnode->v_type == VREG) {
    return -1;
  } else if (curvnode->v_type == VDIR) {
    struct direntry *entry = (struct direntry *)node->direntry;
    assert(entry);
    for (size_t i = 0; i < entry->cnt; i++) {
      assert(entry->nodes[i]->name);
      if (strcmp(entry->nodes[i]->name, name) == 0) {
        *res = entry->nodes[i]->node;
        return 0;
      }
    }

    return ENOENT;
  }
  return ENOENT;
}

static int mount(struct vfs *curvfs, char *path, void *data) {
  struct tmpfsnode *dir = (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  struct direntry *direntry =
      (struct direntry *)kmalloc(sizeof(struct direntry));
  dir->direntry = direntry;
  direntry->nodes = NULL;
  direntry->cnt = 0;
  dir->name = "root";
  dir->size = 0;
  struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
  newnode->ops = &tmpfs_ops;
  newnode->data = dir;
  newnode->v_type = VDIR;
  newnode->stat.st_mode = S_IFDIR;
  newnode->vfs = curvfs;
  struct tmpfsnode *dot = (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  dot->name = ".";
  dot->size = 0;
  dot->node = newnode;
  dot->direntry = direntry;
  struct tmpfsnode *dotdot =
      (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  dotdot->name = "..";
  dotdot->size = 0;
  dotdot->node = newnode;
  dotdot->direntry = direntry;
  insert_into_list(dot, direntry);
  insert_into_list(dotdot, direntry);
  curvfs->cur_vnode = newnode;
  dir->node = newnode;
  kprintf("tmpfs(): created and mounted\r\n");
  return 0;
}
static size_t rw(struct vnode *curvnode, size_t offset, size_t size,
                 void *buffer, int rw, struct FileDescriptorHandle *hnd,
                 int *res) {
  if (curvnode->v_type == VDIR) {
    *res = EISDIR;
    return 0;
  }
  struct tmpfsnode *bro = curvnode->data;
  if (rw == 0) {
    if (offset >= bro->size) {
      return 0;
    }
    size_t count = bro->size - offset;
    if (count > size) {
      count = size;
    }
    memcpy(buffer, bro->data + offset, count);
    return count;
  } else {
    if (offset + size > bro->size) {
      void *new_buf = kmalloc(offset + size);
      if (bro->data) {
        memcpy(new_buf, bro->data, bro->size);
        kfree(bro->data, bro->size);
      }
      bro->data = new_buf;
      bro->size = offset + size;
      bro->node->stat.size = offset + size;
    }
    memcpy((void *)((uint64_t)bro->data + offset), buffer, size);
    return size;
  }
}
static int ioctl(struct vnode *curvnode, unsigned long request, void *arg,
                 void *result) {
  return ENOSYS;
}
static int poll(struct vnode *curvnode, struct pollfd *requested) {
  requested->revents =
      requested->events; // we are able to be read without blocking
  // cause i am literarly in ram :skull:
  return 0;
}
int hardlink(struct vnode *curvnode, struct vnode *with, const char *name) {
  if (curvnode->v_type != VDIR) {
    return EINVAL;
  }
  struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
  assert(node);
  struct direntry *entry = (struct direntry *)node->direntry;
  assert(entry);
  struct tmpfsnode *dir = (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  dir->node = with;
  dir->name = (char *)name;
  insert_into_list(dir, entry);
  return 0;
}
static int open(struct vnode *curvnode, int flags, unsigned int mode, int *res) {
  int fd = alloc_fd_struct(curvnode);
  struct FileDescriptorHandle *hnd = get_fd(fd);
  hnd->flags = flags;
  *res = 0;
  hnd->mode = mode;
  return fd;
}
static struct dirstream *getdirents(struct vnode *curvnode, int *res) {
  if (curvnode->v_type != VDIR) {
    *res = ENOTDIR;
    return NULL;
  }
  struct tmpfsnode *imopid = (struct tmpfsnode*)curvnode->data;

  struct direntry *a = (struct direntry*)imopid->direntry;
  struct dirstream *star = (struct dirstream *)kmalloc(sizeof(struct dirstream));
  star->list = (struct linux_dirent64 *)kmalloc((a->cnt + 1) * sizeof(struct linux_dirent64));
  star->cnt = a->cnt;
  size_t index = 0;
  while (index != a->cnt) {
    struct tmpfsnode *n = a->nodes[index];
    struct linux_dirent64 *m = &star->list[index];
    memcpy(m->d_name, n->name, strlen(n->name) + 1);
    m->d_ino = 0;
    m->d_reclen = sizeof(struct linux_dirent64);
    m->d_off = (index + 1) * sizeof(struct linux_dirent64);
    switch (n->node->v_type) {
      case VREG:
        m->d_type = DT_REG;
        break;
      case VSYMLINK:
        m->d_type = DT_LNK;
        break;
      case VCHRDEVICE:
        m->d_type = DT_CHR;
        break;
      case VBLKDEVICE:
        m->d_type = DT_BLK;
        break;
      case VDIR:
        m->d_type = DT_DIR;
      case VFIFO:
        m->d_type = DT_FIFO;
        break;
    }

    index += 1;
  }
  *res = 0;
  return star;
}
static int close(struct vnode *curvnode, struct FileDescriptorHandle *hnd) {
  if (hnd == NULL) {
    return EBADF;
  }
  return 0;
}