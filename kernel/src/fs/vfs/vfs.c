#include "vfs.h"

#include "utils/basic.h"
#define DOTDOT 1472
#define DOT	   46
struct vfs root = {.vfs_next = NULL, .vfs_ops = NULL, .cur_vnode = NULL};
struct vnode* vfs_lookup(struct vnode* start, char* path)
{
	struct vnode* starter = start;
	if (path[0] == '/')
	{
		// assume root
		starter = root.cur_vnode;
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
	vfs_lookup(NULL, "/meow/.././");
}
