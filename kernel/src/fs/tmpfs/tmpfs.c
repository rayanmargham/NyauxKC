#include "tmpfs.h"

#include <fs/vfs/vfs.h>
#include <mem/kmem.h>

static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnode **res, void *data);
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
    struct direntry *entry = (struct direntry *)node->data;
    node = entry->siblings;
    if (node != NULL) {
      while (offset != 0) {
        if (node->next == NULL) {
          return -1;
        }
        offset -= 1;

        node = node->next;
      }
      *name = node->name;
      return 0;
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}
static int create(struct vnode *curvnode, char *name, enum vtype type,
                  struct vnode **res, void *data) {
  if (curvnode->v_type == VDIR) {
    struct tmpfsnode *node = (struct tmpfsnode *)curvnode->data;
    struct direntry *entry = (struct direntry *)node->data;
    node = entry->siblings;
    struct tmpfsnode *prev = NULL;
    while (node != NULL) {
      if (node->next == NULL) {
        prev = node;
      }
      node = node->next;
    }
    if (type == VDIR) {
      struct tmpfsnode *dir =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct tmpfsnode *dot =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct tmpfsnode *dotdot =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct direntry *direntry =
          (struct direntry *)kmalloc(sizeof(struct direntry));
      dir->data = direntry;
      dir->name = name;
      dir->next = NULL;
      dir->size = 0;
      dir->next = dot;

      dot->data = dir;
      dot->name = ".";
      dot->size = 0;
      dot->next = dotdot;
      dot->data = dir;

      dotdot->data = node;
      dotdot->name = "..";
      dotdot->node = curvnode;
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
      newnode->data = dir;
      newnode->v_type = VDIR;
      newnode->ops = &tmpfs_ops;
      newnode->vfs = curvnode->vfs;
      dot->node = newnode;
      dir->node = newnode;

      if (prev == NULL) {
        entry->siblings = dir;
      } else {
        prev->next = dir;
      }
      *res = newnode;
      // kprintf("tmpfs(): created\r\n");
      return 0;
    } else {
      struct tmpfsnode *file =
          (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
      struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));

      newnode->v_type = type;
      newnode->ops = &tmpfs_ops;
      newnode->vfs = curvnode->vfs;
      newnode->data = file;
      file->name = name;
      file->size = 0;
      newnode->stat.size = 0;
      if (prev == NULL) {
        entry->siblings = file;
      } else {
        prev->next = file;
      }
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

    node = entry->siblings;
    while (node != NULL) {
      if (strcmp(node->name, name) == 0) {

        *res = node->node;
        return 0;
      }
      node = node->next;
    }
    // kprintf("tmpfs(): nothing found\r\n");
  }
  return -1;
}
static int mount(struct vfs *curvfs, char *path, void *data) {
  struct tmpfsnode *dir = (struct tmpfsnode *)kmalloc(sizeof(struct tmpfsnode));
  struct direntry *direntry =
      (struct direntry *)kmalloc(sizeof(struct direntry));
  dir->data = direntry;
  dir->name = "root";
  dir->next = NULL;
  dir->size = 0;
  dir->next = NULL;
  struct vnode *newnode = (struct vnode *)kmalloc(sizeof(struct vnode));
  newnode->ops = &tmpfs_ops;
  newnode->data = dir;
  newnode->v_type = VDIR;
  newnode->vfs = curvfs;
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
