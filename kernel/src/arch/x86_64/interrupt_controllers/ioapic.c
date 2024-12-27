#include "ioapic.h"

#include <stdint.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>

#include "uacpi/platform/compiler.h"
#include "uacpi/status.h"
#include "utils/basic.h"
// warning
// all mmio operations SHOULD be volatile
// making them not will cause UB.

struct acpi_madt_interrupt_source_override isos[16];
struct acpi_madt_ioapic apics[20];	  // no one is gonna have more then 20 ioapics :skull:
uint32_t ioapic_read(struct acpi_madt_ioapic* apic, uint64_t offset)
{
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + offset);
	return *(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + 0x10);
}
void ioapic_write(struct acpi_madt_ioapic* apic, uint64_t offset, uint32_t val)
{
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + offset);
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + 0x10) = val;
}
void populate_ioapic()
{
	// volatile int* x = 0;
	// *x = 6;
	struct uacpi_table l;
	uacpi_status ret = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &l);
	if (uacpi_unlikely(ret != UACPI_STATUS_OK))
	{
		panic("Failed to find the MADT\n");
	}
	struct acpi_madt* hehe = (struct acpi_madt*)l.virt_addr;
	uint64_t length = hehe->hdr.length - sizeof(*hehe);
	size_t offset = 0;
	int ioapiccount = 0;
	int isocount = 0;
	while (offset != length)
	{
		struct acpi_entry_hdr* ent = (void*)hehe->entries + offset;
		switch (ent->type)
		{
			case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:

				struct acpi_madt_interrupt_source_override* ohcool = (struct acpi_madt_interrupt_source_override*)ent;
				assert(isocount < 16);
				isos[isocount] = *ohcool;
				kprintf("populate_ioapic(): isa override from isa %d to gsi %d\n", ohcool->source, ohcool->gsi);
				break;
			case ACPI_MADT_ENTRY_TYPE_IOAPIC:
				struct acpi_madt_ioapic* blah = (struct acpi_madt_ioapic*)ent;
				kprintf("populate_ioapic(): found ioapic %d, manages from gsi %d, phys address %lx\n", ioapiccount,
						blah->gsi_base, (uint64_t)blah->address);
				kprintf("populate_ioapic(): ioapic version is %d. max gsi is %d\n", 0xF & (24 >> ioapic_read(blah, 0)),
						0xFF & (16 >> ioapic_read(blah, 1)));
				assert(ioapiccount < 20);
				apics[ioapiccount] = *blah;
				ioapiccount += 1;
				break;
			case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI: struct acpi_madt_lapic_nmi* e = (struct acpi_madt_lapic_nmi*)ent;

			default: break;
		}
		offset += ent->length;
	}
}
