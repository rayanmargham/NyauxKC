#pragma once
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
struct tmpfsnode
{
	struct vnode* node;
	char* name;
	size_t size;
	struct tmpfsnode* next;
	void* data;	   // dir entry would be stored here if it was a directory
};
struct direntry
{
	struct tmpfsnode* siblings;	   // dir -> file -> dir -> file
};
extern struct vnodeops tmpfs_ops;
extern struct vfs_ops tmpfs_vfsops;
