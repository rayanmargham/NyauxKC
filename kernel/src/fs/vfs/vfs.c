#include "vfs.h"

#include <mem/kmem.h>

#include "fs/tmpfs/tmpfs.h"
#include "utils/basic.h"

#define DOTDOT 1472
#define DOT	   46
struct vfs* vfs_list = NULL;
int vfs_mount(struct vfs_ops ops, char* path, void* data)
{
	struct vfs* o = kmalloc(sizeof(struct vfs));
	o->vfs_ops = &ops;
	o->vfs_ops->mount(o, path, data);
	if (vfs_list == NULL)
	{
		vfs_list = o;
		return 0;
	}
	else
	{
		return -1;	  // TODO, IMPL THIS
	}
}
struct vnode* vfs_lookup(struct vnode* start, char* path)
{
	struct vnode* starter = start;
	if (path[0] == '/')
	{
		// assume root
		starter = vfs_list->cur_vnode;
	}
	char* token;
	token = strtok(path, "/");
	while (token != NULL)
	{
		kprintf("vfs(): %s -> %lu\r\n", token, str_hash(token));
		if (starter == NULL || starter->ops == NULL)
		{
			kprintf("vfs(): cannot resolve path as vnode operations are NULL\r\n");
			return NULL;
		}
		switch (str_hash(token))
		{
			case DOTDOT:
				// TODO
				break;
			case DOT: break;
			default:
				struct vnode* res = NULL;
				int ress = starter->ops->lookup(starter, token, &res);
				if (ress != 0)
				{
					return NULL;
				}

				starter = res;
				break;
		}
		token = strtok(NULL, "/");
	}
	return starter;
}
void vfs_init()
{
	vfs_mount(tmpfs_vfsops, NULL, NULL);
	struct vnode* fein;
	vfs_list->cur_vnode->ops->create(vfs_list->cur_vnode, "meow", VDIR, &fein);
	vfs_lookup(NULL, "/meow");

	// vfs_lookup(NULL, "/meow/.././");
}
