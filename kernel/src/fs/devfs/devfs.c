#include "devfs.h"

#include <fs/tmpfs/tmpfs.h>
#include <utils/libc.h>

#include "dev/null/null.h"
#include "fs/vfs/vfs.h"

static int create(struct vnode* curvnode, char* name, enum vtype type, struct vnode** res, void* data);
static int lookup(struct vnode* curvnode, char* name, struct vnode** res);
static size_t rw(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw);
static int readdir(struct vnode* curvnode, int offset, char** name);
static int mount(struct vfs* curvfs, char* path, void* data);
struct vnodeops vnode_devops = {.lookup = lookup, .create = create, .rw = rw, readdir};
struct vfs_ops vfs_devops = {.mount = mount};
static int mount(struct vfs* curvfs, char* path, void* data)
{
	devnull_init(curvfs);
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
			starter->ops->create(starter, token, VDIR, &ret, NULL);
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

int create(struct vnode* curvnode, char* name, enum vtype type, struct vnode** res, void* data)
{
	if (type == VREG)
	{
		struct devfsinfo* info = (struct devfsinfo*)data;
		struct devfsnode* node = (struct devfsnode*)curvnode->data;
		struct devfsdirentry* entry = (struct devfsdirentry*)node->data;
		node = entry->siblings;
		struct devfsnode* prev = NULL;
		while (node != NULL)
		{
			if (node->next == NULL)
			{
				prev = node;
			}
			node = node->next;
		}
		struct devfsnode* devnode = (struct devfsnode*)kmalloc(sizeof(struct devfsnode));
		struct vnode* vnode = (struct vnode*)kmalloc(sizeof(struct vnode));
		vnode->v_type = type;
		vnode->data = devnode;
		vnode->ops = &vnode_devops;
		vnode->vfs = curvnode->vfs;
		devnode->curvnode = vnode;
		devnode->major = info->major;
		devnode->minor = info->minor;
		devnode->ops = *info->ops;
		devnode->name = name;

		if (prev == NULL)
		{
			entry->siblings = devnode;
		}
		else
		{
			prev->next = devnode;
		}
		*res = vnode;
		kfree(info, sizeof(struct devfsinfo));
		return 0;
	}
	else if (type == VDIR)
	{
		struct devfsnode* node = (struct devfsnode*)curvnode->data;
		struct devfsdirentry* entry = (struct devfsdirentry*)node->data;
		node = entry->siblings;
		struct devfsnode* prev = NULL;
		while (node != NULL)
		{
			if (node->next == NULL)
			{
				prev = node;
			}
			node = node->next;
		}
		struct devfsnode* devnode = (struct devfsnode*)kmalloc(sizeof(struct devfsnode));
		struct devfsdirentry* direntryyy = (struct devfsdirentry*)kmalloc(sizeof(struct devfsdirentry*));
		struct vnode* vnode = (struct vnode*)kmalloc(sizeof(struct vnode));
		vnode->v_type = type;
		vnode->data = devnode;
		vnode->ops = &vnode_devops;
		vnode->vfs = curvnode->vfs;
		devnode->curvnode = vnode;
		devnode->major = 0;
		devnode->minor = 0;
		devnode->name = name;
		devnode->data = direntryyy;

		if (prev == NULL)
		{
			entry->siblings = devnode;
		}
		else
		{
			prev->next = devnode;
		}
		*res = vnode;
		return 0;
	}
	return -1;
}
static size_t rw(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw)
{
	struct devfsnode* devnode = (struct devfsnode*)curvnode->data;
	return devnode->ops.rw(curvnode, offset, size, buffer, rw);
}
static int lookup(struct vnode* curvnode, char* name, struct vnode** res)
{
	struct devfsnode* node = (struct devfsnode*)curvnode->data;
	if (curvnode->v_type == VREG)
	{
		return -1;
	}
	else if (curvnode->v_type == VDIR)
	{
		struct devfsdirentry* entry = (struct devfsdirentry*)node->data;

		node = entry->siblings;
		while (node != NULL)
		{
			if (strcmp(node->name, name) == 0)
			{
				*res = node->curvnode;
				return 0;
			}
			node = node->next;
		}
		// kprintf("tmpfs(): nothing found\r\n");
	}
	return -1;
}
static int readdir(struct vnode* curvnode, int offset, char** name)
{
	return -1;
}
