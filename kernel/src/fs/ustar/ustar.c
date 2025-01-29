#include "ustar.h"

#include "limine.h"
#include "term/term.h"
#include "utils/basic.h"

// stolen from https://wiki.osdev.org/USTAR :trl:
int oct2bin(unsigned char* str, int size)
{
	int n = 0;
	unsigned char* c = str;
	while (size-- > 0)
	{
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

void populate_tmpfs_from_tar()
{
	kprintf("response: %lu\r\n", modules.id);
	if (modules.response->module_count == 0)
	{
		panic("populate_tmpfs_from_tar(): Nyaux Cannot Continue without a initramfs...\r\n");
	}
	unsigned char* ptr = modules.response->modules[0]->address;
}
