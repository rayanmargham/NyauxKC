#include "hpet.h"

#include <stdint.h>

#include "mem/pmm.h"
#include "term/term.h"
#include "uacpi/acpi.h"
#include "uacpi/status.h"
#include "uacpi/tables.h"

uint64_t ctr_clock_period = 0;	  // in nano seconds
volatile uint64_t* hpetvirtaddr = NULL;
bool bit32 = false;
void stall_with_hpetclk(uint64_t ms)
{
	if (bit32 == false)
	{
		volatile uint64_t pol_start = *(volatile uint64_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		volatile uint64_t* pol_cur = (volatile uint64_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		while ((*pol_cur - pol_start) * ctr_clock_period < ms * 1000000)
		{
		}
	}
	else
	{
		volatile uint64_t pol_start = *(volatile uint32_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		volatile uint32_t* pol_cur = (volatile uint32_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		while ((*pol_cur - pol_start) * ctr_clock_period < ms * 1000000)
		{
		}
	}
}
void stall_with_hpetclkmicro(uint64_t usec)
{
	if (bit32 == false)
	{
		volatile uint64_t pol_start = *(volatile uint64_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		volatile uint64_t* pol_cur = (volatile uint64_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		while ((*pol_cur - pol_start) * ctr_clock_period < usec * 1000)
		{
		}
	}
	else
	{
		volatile uint64_t pol_start = *(volatile uint32_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		volatile uint32_t* pol_cur = (volatile uint32_t*)((volatile uint64_t)hpetvirtaddr + 0xf0);
		while ((*pol_cur - pol_start) * ctr_clock_period < usec * 1000)
		{
		}
	}
}
uint64_t read_hpet_counter()
{
	assert(hpetvirtaddr != NULL);
	if (bit32 == false)
	{
		return (*(volatile uint64_t*)((volatile uint64_t)hpetvirtaddr + 0xf0) * ctr_clock_period);
	}
	else
	{
		return (*(volatile uint32_t*)((volatile uint64_t)hpetvirtaddr + 0xf0) * ctr_clock_period);
	}
}
void init_hpet()
{
	if (hpetvirtaddr != NULL)
	{
		return;
	}
	uacpi_table hpet_table;
	uacpi_status st = uacpi_table_find_by_signature("HPET", &hpet_table);
	if (st != UACPI_STATUS_OK)
	{
		panic("The Nyaux Kernel does not support Devices without a HPET\r\nThe "
			  "Reason for this is becausei hate tsc with a burning desire. pr in a "
			  "different calibration timer if u want one so bad lol");
	}
	kprintf("init_hpet(): Timer Table Found\r\n");
	struct acpi_hpet* hpet = (struct acpi_hpet*)hpet_table.virt_addr;
	kprintf("init_hpet(): HPET has Physical Address 0x%lx\r\n", hpet->address.address);
	volatile uint32_t* cap = (volatile uint32_t*)(hpet->address.address + hhdm_request.response->offset);
	if (!(*cap & (1 << 13)))
	{
		kprintf("init_hpet(): Bit 13 is off\r\ninit_hpet(): Panicking.\r\n");
		bit32 = true;
		volatile uint64_t* capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		ctr_clock_period = ((volatile uint32_t)(*capr >> 32)) / 1000000;
		kprintf("init_hpet(): Counter Clock Period %lu\r\n", ctr_clock_period);
		capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset + 0x10);
		*capr |= 1;	   // enable counter

		kprintf("init_hpet(): Main Counter Enabled!\r\n");
		hpetvirtaddr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		bit32 = true;
	}
	else
	{
		volatile uint64_t* capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		ctr_clock_period = ((volatile uint64_t)(*capr >> 32)) / 1000000;
		kprintf("init_hpet(): Counter Clock Period %lu\r\n", ctr_clock_period);
		capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset + 0x10);
		*capr |= 1;	   // enable counter

		kprintf("init_hpet(): Main Counter Enabled!\r\n");
		hpetvirtaddr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
	}
}
