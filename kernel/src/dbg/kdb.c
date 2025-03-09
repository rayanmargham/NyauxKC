#include "kdb.h"

#include "mem/vmm.h"
#include "sched/sched.h"
#include "term/term.h"
bool serial_data_ready()
{
	return (inb(0x3F8 + 5) & 0b1);
}
void rsh()
{
	char buf[256];
	int idx = 0;
	kprintf("(rsh): ");
	inb(0x3F8);
	while (true)
	{
		if (serial_data_ready())
		{
			char got = (char)inb(0x3F8);
			if (got == 127 && idx > 0)
			{
				kprintf("\b \b");

				buf[idx] = '\0';
				idx--;
			}
			else if (got == '\r')
			{
				kprintf("\r\n");
				buf[idx] = '\0';

				for (int i = 0; i != 4; i++)
				{
					if (strcmp(buf, cmds[i]) == 0)
					{
						switch (i)
						{
							case 0: goto t; break;
							case 1:
								kprintf_all_vmm_regions();
								goto r;
								break;
							default:
								kprintf("commands to do are stubs:c\r\n");
								goto r;
								break;
						}
					}
				}
				kprintf("not a valid command\r\n");
r:
				kprintf("(rsh): ");

				idx = 0;
				buf[idx] = '\0';
				// exit_thread();
				// break;
			}
			else if (got == 127)
			{
			}
			else
			{
				buf[idx] = got;

				kprintf("%c", got);
				idx++;
			}
		}
	}
t:
	// exit_thread();
}
