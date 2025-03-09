#include "null.h"

// impl open() later on
struct devfsops nullops = {.rw = rw};
static size_t rw(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw)
{
	if (rw)
	{
		return 0;
	}
	else
	{
		return size;
	}
}
void devnull_init(struct vfs* curvfs)
{
	struct vnode* res;
	struct devfsinfo* info = kmalloc(sizeof(struct devfsinfo));
	info->major = 1;
	info->minor = 0;
	info->ops = &nullops;
	curvfs->cur_vnode->ops->create(curvfs->cur_vnode, "null", VREG, &res, info);
}
