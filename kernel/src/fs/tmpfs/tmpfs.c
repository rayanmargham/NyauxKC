#include "tmpfs.h"

#include <fs/vfs/vfs.h>
#include <mem/kmem.h>

int create(struct vnode* curvnode, char* name, struct vnode** res)
{
	kprintf("tmpfs(): attempting to create file/dir with name %s\r\n", name);
	struct tmpfsnode* node = kmalloc(sizeof(struct tmpfsnode));
	node->name = name;
	node->node = curvnode;
	if (node->node->v_type == VDIR)
	{
		struct direntry* entry = kmalloc(sizeof(struct direntry));
		kfree(entry->siblings, sizeof(struct tmpfsnode));
		node->data = (void*)entry;
		kprintf("tmpfs(): Directory Created with Name %s\r\n", name);
		return 0;
	}
	else
	{
		//  TODO
	}
	return -1;
}

int lookup(struct vnode* curvnode, char* name, struct vnode** res)
{
}
