#include "acpi.h"

#include "mem/pmm.h"
#include "term/term.h"
#include "timers/hpet.h"
#include "uacpi/event.h"

void init_acpi()
{
	uint64_t rsdp = (uint64_t)rsdp_request.response->address - hhdm_request.response->offset;
	kprintf("init_acpi(): initing uacpi\n");
	uacpi_status st = uacpi_initialize(0);
	init_hpet();
	kprintf("init_acpi(): now loading namespace\n");
	if (st == UACPI_STATUS_OK)
	{
		st = uacpi_namespace_load();
		st = uacpi_namespace_initialize();
		st = uacpi_finalize_gpe_initialization();

		kprintf("init_acpi(): uacpi finished\n");
	}
	else
	{
		panic("init_acpi(): Failed\n");
	}
}
