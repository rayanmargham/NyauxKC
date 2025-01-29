#include "devfs.h"

#include <utils/libc.h>

#include "fs/vfs/vfs.h"

static int create(struct vnode* curvnode, char* name, enum vtype type, struct vnode** res);
static int lookup(struct vnode* curvnode, char* name, struct vnode** res);
static size_t rw(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw);
static int mount(struct vfs* curvfs, char* path, void* data);
struct vnodeops vnode_devops = {.lookup = lookup, .create = create, .rw = rw};
struct vfs_ops vfs_devops = {.mount = mount};
static int mount(struct vfs* curvfs, char* path, void* data)
{
	return 0;
}
void devfs_init(struct vfs* curvfs)
{
	char* token;
	token = strtok("/dev", "/");
	struct vnode* starter = curvfs->cur_vnode;
	while (token != NULL)
	{
		struct vnode* ret;
		int result = starter->ops->lookup(starter, token, &ret);
		if (result == -1)
		{
			starter->ops->create(starter, token, VDIR, &ret);
			starter = ret;
		}
		else
		{
			starter = ret;
		}
		token = strtok(NULL, "/");
	}
	kprintf("okay\r\n");
	struct vfs* new = kmalloc(sizeof(struct vfs));
	new->cur_vnode = starter;
	new->cur_vnode->ops = &vnode_devops;
	new->vfs_ops = &vfs_devops;
	struct vfs* old = curvfs;
	while (true)
	{
		if (old->vfs_next == NULL)
		{
			break;
		}
		old = old->vfs_next;
	}
	old->vfs_next = new;
	new->vfs_ops->mount(new, NULL, NULL);
}
int create(struct vnode* curvnode, char* name, enum vtype type, struct vnode** res)
{
	switch (type)
	{
		case VREG: break;
		default: return -1;
	}
	return -1;
}
