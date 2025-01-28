#pragma once
#include <stddef.h>
#include <term/term.h>
#include <utils/basic.h>
#include <utils/libc.h>
void vfs_init();
enum vtype
{
	VREG,
	VDIR
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
	int (*lookup)(struct vnode* curvnode, const char* name, struct vnode** res);
	int (*create)(struct vnode* curvnode, const char* name, struct vnode** res);
};
struct vfs_ops
{
};
struct vfs
{
	struct vfs* vfs_next;
	struct vfs_ops* vfs_ops;
	struct vnode* cur_vnode;
};
