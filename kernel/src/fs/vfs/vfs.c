#include "vfs.h"

#include <mem/kmem.h>

#include "fs/devfs/devfs.h"
#include "fs/tmpfs/tmpfs.h"
#include "fs/ustar/ustar.h"
#include "sched/sched.h"
#include "term/term.h"
#include "utils/basic.h"
#include "utils/libc.h"

struct vfs *vfs_list = NULL;
int vfs_mount(struct vfs_ops *ops, char *path, void *data) {
  struct vfs *o = kmalloc(sizeof(struct vfs));
  o->vfs_ops = ops;
  o->vfs_ops->mount(o, path, data);
  if (vfs_list == NULL) {
    vfs_list = o;
    return 0;
  } else {
    return -1; // TODO, IMPL THIS
  }
}

int vfs_lookup(struct vnode *start, const char *path, struct vnode **node) {
  struct vnode *starter = start;
  sprintf("vfs_lookup(): looking up \"%s\"\r\n", path);
  if (path[1] == '\0' && path[0] != '/') {
    if (path[0] == '.') {
      //
      struct process_t *a = get_process_start();
      assert(a->cwd);
      *node = a->cwd;
      get_process_finish(a);
      return 0;
    }
    if (starter != NULL) {
      struct vnode *result = NULL;
      int res = starter->ops->lookup(starter, (char *)path, &result);
      if (res != 0) {
        kprintf("vfs(): file not found\r\n");
        return res;
      }
      if (result->v_type == VSYMLINK) {
        panic("vfs(): not supported");
      }
      *node = starter;
      return 0;
    } else {
      kprintf("vfs(): file not found\r\n");
      return -1;
    }
  }
  if (path[0] == '/' || starter == NULL) {
    // assume root
    starter = vfs_list->cur_vnode;
    if (path[0] == '/') {
      path += 1;
    }
  }

  while (*path) {
    if (starter->v_type != VDIR) {
      kprintf("vfs(): starter is NOT a dir!!! %d\r\n", starter->v_type);
      return -1;
    }
    if (*path == '/') {
      path += 1;
      continue;
    }
    const char *start = path;
    while (*path && *path != '/') {
      path += 1;
    }
    size_t len = path - start;
    if (len == 0) {
      continue;
    }

    char *name = kmalloc(len + 1);
    memcpy(name, start, len);
    name[len] = 0;
    struct vnode *res = NULL;
    int ress = starter->ops->lookup(starter, name, &res);
    kfree(name, len + 1);
    if (ress != 0) {
      sprintf("vfs(): file not found\r\n");
      return ress;
    }
    if (res->v_type == VSYMLINK) {
      ress = vfs_lookup(starter, res->data, &res);
      if (ress != 0) {
        kprintf("vfs(): symlink not found\r\n");
        return ress;
      }
    }
    starter = res;
  }
  *node = starter;
  return 0;
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
                                       &tmpfs_ops, &next_node, NULL,
                                       NULL) == 0);

      if ((type == VREG || type == VSYMLINK) && buf != NULL && filesize != 0) {
        if (type == VSYMLINK) {
          void *buffer = kmalloc(filesize + 1);
          memcpy(buffer, buf, filesize + 1);
          next_node->data = buffer;
        } else {
          int res = 0;
          assert(next_node->ops->rw(next_node, 0, filesize, buf, 1, NULL,
                                    &res) == filesize);
          // asseration will fail so yk
        }
      }
      return;
    }

    if (res != 0)
      assert(current_node->ops->create(current_node, strdup(token), VDIR,
                                       &tmpfs_ops, &next_node, NULL,
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
  vfs_mount(&tmpfs_vfsops, NULL, NULL);
  // vfs_scan();

  populate_tmpfs_from_tar();
  devfs_init(vfs_list);
  // doing this manually cause f you
  struct process_t *m = get_process_start();
  m->cwd = vfs_list->cur_vnode;
  get_process_finish(m);
}
