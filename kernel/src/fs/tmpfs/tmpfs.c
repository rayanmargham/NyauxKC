#include "tmpfs.h"
#include "utils/hashmap.h"

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
static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnodeops *ops, struct vnode **res, void *data);
static int lookup(struct vnode *curvnode, char *name, struct vnode **res);
static size_t rw(struct vnode *curvnode, size_t offset, size_t size,
                 void *buffer, int rw);
static int mount(struct vfs *curvfs, char *path, void *data);
static int readdir(struct vnode *curvnode, int offset, char **name);
struct vnodeops tmpfs_ops = {lookup, create, rw, readdir};
struct vfs_ops tmpfs_vfsops = {mount};
static int readdir(struct vnode *curvnode, int offset, char **name) {
  if (curvnode->v_type == VDIR) {
    struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
    if ((size_t)offset >= node->direntry->cnt) {
      return -1;
    }
    *name = node->direntry->nodes[offset]->name;
    return 0;
  }
  return -1;
}
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
                  struct vnodeops *ops, struct vnode **res, void *data) {
  if (curvnode->v_type == VDIR) {
    struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
    struct direntry *entry = (struct direntry *)node->data;
    if (type == VDIR) {
      struct tmpfsnode *dir =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct direntry *direntry =
          (struct direntry *)kmalloc(sizeof(struct direntry));
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
      newnode->data = dir;
      newnode->v_type = VDIR;
      newnode->ops = ops;
      newnode->vfs = curvnode->vfs;
      dir->direntry = direntry;
      dir->name = name;
      dir->size = 0;
      dir->node = newnode;

      struct tmpfsnode *dot =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      dot->name = ".";
      dot->size = 0;
      dot->node = newnode;
      dot->direntry = direntry;
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
      // kprintf("tmpfs(): created\r\n");
      return 0;
    } else {
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
    struct direntry *entry = (struct direntry *)node->data;
    for (size_t i = 0; i < entry->cnt; i++) {
      if (strcmp(entry->nodes[i]->name, name) == 0) {
        *res = entry->nodes[i]->node;
        return 0;
      }
    }
    return -1;
  }
  return -1;
}
static int mount(struct vfs *curvfs, char *path, void *data) {
  struct tmpfsnode *dir = (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  struct direntry *direntry =
      (struct direntry *)kmalloc(sizeof(struct direntry));
  dir->data = direntry;
  direntry->nodes = NULL;
  direntry->cnt = 0;
  dir->name = "root";
  dir->size = 0;
  struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
  newnode->ops = &tmpfs_ops;
  newnode->data = dir;
  newnode->v_type = VDIR;
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
  kprintf("tmpfs(): created and mounted\r\n");
  return 0;
}
static size_t rw(struct vnode *curvnode, size_t offset, size_t size,
                 void *buffer, int rw) {
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
    memcpy((void *)(bro->data + offset), buffer, size);
    return size;
  }
}
