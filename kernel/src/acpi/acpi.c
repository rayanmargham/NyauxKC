#include "acpi.h"

#include "acpi/ec.h"
#include "arch/x86_64/interrupt_controllers/ioapic.h"
#include "mem/pmm.h"
#include "term/term.h"
#include "timers/timer.hpp"
#include "uacpi/context.h"
#include "uacpi/event.h"
#include "uacpi/types.h"
#include "uacpi/uacpi.h"

void init_acpi_early()
{
	kprintf("init_acpi(): initing uacpi\n");
	// uacpi_context_set_log_level(UACPI_LOG_TRACE);
	uacpi_status st = uacpi_initialize(0);
	GenericTimerInit();
	populate_ioapic();
	initecfromecdt();
	st = uacpi_namespace_load();
	st = uacpi_namespace_initialize();
	ec_init();
	st = uacpi_finalize_gpe_initialization();

	kprintf("init_acpi(): now loading namespace\n");
	if (st == UACPI_STATUS_OK)
	{
		kprintf("init_acpi(): uacpi early init finished\n");
		return;
	}
	else
	{
		panic("init_acpi(): Failed\n");
	}
}
