#include "ioapic.h"

#include <stdint.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>

#include "arch/x86_64/instructions/instructions.h"
#include "uacpi/platform/compiler.h"
#include "uacpi/status.h"
#include "utils/basic.h"

// warning
// all mmio operations SHOULD be volatile
// making them not will cause UB.
// do u wanna die
// read this code :)

struct acpi_madt_interrupt_source_override isos[16];
struct acpi_madt_ioapic apics[20];	  // no one is gonna have more then 20 ioapics :skull:
uint32_t ioapic_read(struct acpi_madt_ioapic* apic, uint64_t offset)
{
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address) = offset;
	return *(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + 0x10);
}
void ioapic_write(struct acpi_madt_ioapic* apic, uint64_t offset, uint32_t val)
{
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address) = offset;
	*(volatile uint32_t*)(hhdm_request.response->offset + (uint64_t)apic->address + 0x10) = val;
}
void print_ioapicflags(uint16_t flags)
{
	// extract delivery mode
	uint8_t delivery_mode = flags & 0x7;
	uint8_t desination_mode = (flags >> 3) & 1;
	uint8_t interrupt_pin_polarity = (flags >> 5) & 1;
	uint8_t remote_irr = (flags >> 6) & 1;
	uint8_t trigger_mode = (flags >> 7) & 1;
	uint8_t interrupt_mask = (flags >> 8) & 1;
	kprintf(
		"delivery mode is %d, desination mode is %d, interrupt pin polarity is %d\r\nremote irr is %d, trigger mode is %d, interrupt mask is %d\r\n",
		delivery_mode, desination_mode, interrupt_pin_polarity, remote_irr, trigger_mode, interrupt_mask);
}
void print_isoflags(uint16_t flags)
{
	uint8_t polarity = flags & 0b11;
	uint8_t trigger = (flags >> 2) & 0b11;
	kprintf("polarity is %d, trigger is %d\r\n", polarity, trigger);
}
int isoflagtobit(uint8_t thing)
{
	thing = thing & 0b11;	 // extract the 2 bits
	kprintf("thing is %b\r\n", thing);
	if (thing == 0b10)
	{
		return -1;
	}
	else if (thing == 0b11)
	{
		return 0b1;
	}
	else if (thing == 0b01)
	{
		return 0b0;
	}
	return 0;
}
#define sciflags (1 << 13) | (1 << 15)
void route_irq(uint8_t irq, uint8_t vec, uint16_t flags, uint32_t lapic_id)
{
	// if there is an iso, find it.
	int i = 0;
	struct acpi_madt_interrupt_source_override* checker = NULL;
	while (i != 16)
	{
		checker = &isos[i];
		if (checker->source == irq)
		{
			break;
		}
		i += 1;
	}

	if (i != 16)
	{
		i = 0;
		kprintf("Found an iso for this\r\n");
		int polarity = isoflagtobit(checker->flags & 0b11);
		int trigger = isoflagtobit((checker->flags >> 2) & 0b11);
		assert(polarity != -1);
		assert(trigger != -1);
		uint64_t assembled_data =
			((uint64_t)lapic_id << 56) | (trigger << 15) | (polarity << 13) | (0 << 11) | (vec & 0xFF);
		uint32_t first_half = assembled_data & 0xFFFFFFFF;
		uint32_t second_half = (assembled_data >> 32) & 0xFFFFFFFF;
		struct acpi_madt_ioapic* responable = NULL;
		while (i != 20)
		{
			responable = &apics[i];
			uint64_t max_gsi = (0xFF & ((uint64_t)ioapic_read(responable, 1)) >> 16);
			kprintf("max gsi is %lu\r\n", max_gsi);
			if (responable->gsi_base <= checker->gsi && (responable->gsi_base + max_gsi > checker->gsi))
			{
				break;
			}
			i += 1;
		}
		if (i == 20)
		{
			panic("tf.");
		}
		ioapic_write(responable, 0x10 + ((checker->gsi - responable->gsi_base) * 2), first_half);
		ioapic_write(responable, 0x10 + ((checker->gsi - responable->gsi_base) * 2) + 1, second_half);
	}
	else
	{
		i = 0;
		if (flags == 0)
		{
			uint64_t assembled_data = ((uint64_t)lapic_id << 56) | (0 << 15) | (0 << 13) | (0 << 11) | (vec & 0xFF);
			uint32_t first_half = assembled_data & 0xFFFFFFFF;
			uint32_t second_half = (assembled_data >> 32) & 0xFFFFFFFF;
			struct acpi_madt_ioapic* responable = NULL;
			while (i != 20)
			{
				responable = &apics[i];
				uint64_t max_gsi = 0xFF & (((uint64_t)ioapic_read(responable, 1)) >> 16);
				kprintf("max gsi is %lu\r\n", max_gsi);
				if (responable->gsi_base <= irq && (responable->gsi_base + max_gsi) > irq)
				{
					break;
				}
				i += 1;
			}
			if (i == 20)
			{
				panic("tf.");
			}
			ioapic_write(responable, 0x10 + ((irq - responable->gsi_base) * 2), first_half);
			ioapic_write(responable, 0x10 + ((irq - responable->gsi_base) * 2) + 1, second_half);
		}
		else
		{
			uint64_t assembled_data = ((uint64_t)lapic_id << 56) | flags | (vec & 0xFF);
			uint32_t first_half = assembled_data & 0xFFFFFFFF;
			uint32_t second_half = (assembled_data >> 32) & 0xFFFFFFFF;
			struct acpi_madt_ioapic* responable = NULL;
			while (i != 20)
			{
				responable = &apics[i];
				uint64_t max_gsi = 0xFF & (((uint64_t)ioapic_read(responable, 1)) >> 16);
				if (responable->gsi_base <= irq && (responable->gsi_base + max_gsi) > irq)
				{
					break;
				}
				i += 1;
			}
			if (i == 20)
			{
				panic("tf.");
			}
			ioapic_write(responable, 0x10 + ((irq - responable->gsi_base) * 2), first_half);
			ioapic_write(responable, 0x10 + ((irq - responable->gsi_base) * 2) + 1, second_half);
		}
	}
}
void populate_ioapic()
{
	// volatile int* x = 0;
	// *x = 6;
	struct uacpi_table l;
	uacpi_status ret = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &l);
	if (uacpi_unlikely(ret != UACPI_STATUS_OK))
	{
		panic("Failed to find the MADT\r\n");
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
				kprintf(__func__ "(): isa override from isa %d to gsi %d\r\n", ohcool->source, ohcool->gsi);
				break;
			case ACPI_MADT_ENTRY_TYPE_IOAPIC:
				struct acpi_madt_ioapic* blah = (struct acpi_madt_ioapic*)ent;
				kprintf(__func__ "(): found ioapic %d, manages from gsi %d, phys address %lx\r\n", ioapiccount,
						blah->gsi_base, (uint64_t)blah->address);
				kprintf(__func__ "(): ioapic version is %d. max gsi is %d\r\n",
						0xF & (24 >> (uint64_t)ioapic_read(blah, 0)), 0xFF & (((uint64_t)ioapic_read(blah, 1)) >> 16));
				assert(ioapiccount < 20);
				apics[ioapiccount] = *blah;
				ioapiccount += 1;
				break;
			case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI: struct acpi_madt_lapic_nmi* e = (struct acpi_madt_lapic_nmi*)ent;

			default: break;
		}
		offset += ent->length;
	}

	// route_irq(9, 56, 0, get_lapic_id());
}
