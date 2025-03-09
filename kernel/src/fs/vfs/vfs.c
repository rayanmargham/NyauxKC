#include "vfs.h"

#include <mem/kmem.h>

#include "fs/devfs/devfs.h"
#include "fs/tmpfs/tmpfs.h"
#include "fs/ustar/ustar.h"
#include "term/term.h"
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
				if (ress != 0 && res == NULL)
				{
					return NULL;
				}
				else if (ress != 0)
				{
					return res;
				}
				if (res->v_type == VSYMLINK)
				{
					return vfs_lookup(starter, res->data);
				}
				starter = res;
				break;
		}
		token = strtok(NULL, "/");
	}
	return starter;
}
void vfs_create_from_tar(char* path, enum vtype type, size_t filesize, void* buf)
{
	struct vnode* node = vfs_list->cur_vnode;
	struct vnode* starter = node;
	char* token;

	token = strtok(path, "/");
	while (token != NULL)
	{
		struct vnode* epic = NULL;
		int res = starter->ops->lookup(starter, token, &epic);
		if (res != 0)
		{
			kprintf("vfs(): i need to create the thing %s\r\n", token);
			starter->ops->create(starter, token, type, &epic, NULL);
			starter = epic;

			if (type == VREG && buf != NULL && filesize != 0)
			{
				if (starter->v_type == VDIR)
				{
					kprintf("WTF\r\n");
				}
				kprintf("vfs(): writing to file\r\n");
				starter->ops->rw(starter, 0, filesize, buf, 1);
			}
		}
		else
		{
			kprintf("vfs(): i found something\r\n");
			starter = epic;
		}
		token = strtok(NULL, "/");
	}
}
void vf_scan(struct vnode* curvnode)
{
	struct vnode* fein = curvnode;
	int offset = 0;
	int res = 0;
	while (res == 0)
	{
		if (fein->v_type == VDIR)
		{
			char* name;

			int res = fein->ops->readdir(fein, offset, &name);
			if (res != 0)
			{
				break;
			}
			kprintf("->%s\r\n", name);
			offset += 1;
		}
	}
}
void vfs_scan()
{
	struct vnode* fein = vfs_list->cur_vnode;
	vf_scan(fein);
}
void vfs_init()
{
	vfs_mount(tmpfs_vfsops, NULL, NULL);
	struct vnode* fein;
	vfs_list->cur_vnode->ops->create(vfs_list->cur_vnode, "meow", VDIR, &fein, NULL);
	vfs_lookup(NULL, "/meow");
	populate_tmpfs_from_tar();
	struct vnode* res = vfs_lookup(NULL, "/idk");
	char* buffer;
	res->ops->readdir(res, 1, &buffer);
	kprintf("found %s\r\n", buffer);
	devfs_init(vfs_list);
}
