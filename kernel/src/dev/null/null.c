#include "null.h"
// impl open() later on
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
