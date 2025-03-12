#include "vfs.h"

#include <mem/kmem.h>

#include "fs/devfs/devfs.h"
#include "fs/tmpfs/tmpfs.h"
#include "fs/ustar/ustar.h"
#include "term/term.h"
#include "utils/basic.h"
#include "utils/libc.h"

#define DOTDOT 1472
#define DOT 46
struct vfs *vfs_list = NULL;
int vfs_mount(struct vfs_ops ops, char *path, void *data) {
  struct vfs *o = kmalloc(sizeof(struct vfs));
  o->vfs_ops = &ops;
  o->vfs_ops->mount(o, path, data);
  if (vfs_list == NULL) {
    vfs_list = o;
    return 0;
  } else {
    return -1; // TODO, IMPL THIS
  }
}
struct vnode *vfs_lookup(struct vnode *start, char *path) {
  struct vnode *starter = start;
  if (path[0] == '/') {
    // assume root
    starter = vfs_list->cur_vnode;
  }
  char *token;
  token = strtok(path, "/");
  while (token != NULL) {
    kprintf("vfs(): %s -> %lu\r\n", token, str_hash(token));
    if (starter == NULL || starter->ops == NULL) {
      kprintf("vfs(): cannot resolve path as vnode operations are NULL\r\n");
      return NULL;
    }
    switch (str_hash(token)) {
    case DOTDOT:
      // TODO
      break;
    case DOT:
      break;
    default:
      struct vnode *res = NULL;
      int ress = starter->ops->lookup(starter, token, &res);
      if (ress != 0 && res == NULL) {
        return NULL;
      } else if (ress != 0) {
        return res;
      }
      if (res->v_type == VSYMLINK) {
        return vfs_lookup(starter, res->data);
      }
      starter = res;
      break;
    }
    token = strtok(NULL, "/");
  }
  return starter;
}
void vfs_create_from_tar(char *path, enum vtype type, size_t filesize,
                         void *buf) {

  struct vnode *node = vfs_list->cur_vnode;
  assert(node->v_type == VDIR);

  char *token = strtok(path, "/");
  struct vnode *current_node = node;
  while (token != NULL) {
    char *next_token = strtok(NULL, "/");

    struct vnode *next_node = NULL;
    int res = current_node->ops->lookup(current_node, token, &next_node);

    if (next_token == NULL) {
      assert(res != 0);
      assert(current_node->ops->create(current_node, strdup(token), type,
                                       &next_node, NULL) == 0);

      if (type == VREG && buf != NULL && filesize != 0)
        assert(next_node->ops->rw(next_node, 0, filesize, buf, 1) == filesize);
      return;
    }

    if (res != 0)
      assert(current_node->ops->create(current_node, token, VDIR, &next_node,
                                       NULL) == 0);
    assert(next_node->v_type == VDIR);

    current_node = next_node;
    token = next_token;
  }
}
void vf_scan(struct vnode *curvnode) {
  struct vnode *fein = curvnode;
  int offset = 0;
  int res = 0;
  while (res == 0) {
    if (fein->v_type == VDIR) {
      char *name;

      int res = fein->ops->readdir(fein, offset, &name);
      if (res != 0) {
        break;
      }
      kprintf("->%s\r\n", name);

      offset += 1;
    }
  }
}
void vfs_scan() {
  struct vnode *fein = vfs_list->cur_vnode;
  vf_scan(fein);
}
void vfs_init() {
  vfs_mount(tmpfs_vfsops, NULL, NULL);
  struct vnode *fein;
  vfs_scan();

  populate_tmpfs_from_tar();
  devfs_init(vfs_list);
}
