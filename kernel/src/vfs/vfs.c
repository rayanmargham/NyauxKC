#include "vfs.h"

void vfs_init()
{
	char* test = "../.home/meow/e/";
	char* token;
	token = strtok(test, "/");
	while (token != NULL)
	{
		kprintf("vfs(): %s\n", token);
		token = strtok(NULL, "/");
	}
}
