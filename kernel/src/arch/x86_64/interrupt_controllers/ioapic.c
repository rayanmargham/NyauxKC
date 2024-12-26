#include "ioapic.h"

#include <stdint.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>

#include "uacpi/platform/compiler.h"
#include "uacpi/status.h"
#include "utils/basic.h"

void populate_ioapic()
{
	volatile int* x = 0;
	*x = 6;
	struct uacpi_table l;
	uacpi_status ret = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &l);
	if (uacpi_unlikely(ret != UACPI_STATUS_OK))
	{
		panic("Failed to find the MADT\n");
	}
	struct acpi_madt* hehe = (struct acpi_madt*)l.virt_addr;
	uint64_t length = hehe->hdr.length - sizeof(*hehe);
	size_t offset = 0;
	while (offset < length)
	{
		struct acpi_entry_hdr* ent = (void*)hehe->entries + offset;
		switch (ent->type)
		{
			case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:

				struct acpi_madt_interrupt_source_override* ohcool = (struct acpi_madt_interrupt_source_override*)ent;
				kprintf("populate_ioapic(): isa override from isa %d to gsi %d\n", ohcool->source, ohcool->gsi);
				break;
			case ACPI_MADT_ENTRY_TYPE_IOAPIC: struct acpi_madt_ioapic* blah = (struct acpi_madt_ioapic*)ent; break;
			case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI: struct acpi_madt_lapic_nmi* e = (struct acpi_madt_lapic_nmi*)ent;

			default: break;
		}
		offset += ent->length;
	}
}
