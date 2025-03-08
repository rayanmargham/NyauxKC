#pragma once
#include <fs/vfs/vfs.h>
#include <mem/kmem.h>
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>

struct devfsops
{
	size_t (*rw)(struct vnode* curvnode, size_t offset, size_t size, void* buffer, int rw);
};

struct devfsnode
{
	struct vnode* curvnode;
	uint8_t major;
	uint8_t minor;
	struct devfsops ops;
	struct devfsnode* next;
	void* data;
	char* name;
};
extern struct vnodeops vnode_devops;
extern struct vfs_ops vfs_devops;
void devfs_init(struct vfs* curvfs);
struct devfsinfo
{
	uint8_t major;
	uint8_t minor;
	struct devfsops* ops;

	// .... anyhting else
};
struct devfsdirentry
{
	struct devfsnode* siblings;
};
