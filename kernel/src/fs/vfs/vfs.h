#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
void vfs_init();
enum vtype
{
	VREG,
	VDIR,
	VSYMLINK	// symlink
};

struct vnode
{
	struct vfs* vfs;
	struct vnodeops* ops;
	enum vtype v_type;
	void* data;
};
struct vnodeops
{
	int (*lookup)(struct vnode* curvnode, char* name, struct vnode** res);
	int (*create)(struct vnode* curvnode, char* name, enum vtype type, struct vnode** res);
	size_t (*rw)(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw);
};
struct vfs_ops
{
	int (*mount)(struct vfs* curvfs, char* path, void* data);
};
struct vfs
{
	struct vfs* vfs_next;
	struct vfs_ops* vfs_ops;
	struct vnode* cur_vnode;
};
void vfs_create_from_tar(char* path, enum vtype type, size_t filesize, void* buf);
