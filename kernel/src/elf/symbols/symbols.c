
#include "symbols.h"

#include <stdint.h>

#include "mem/pmm.h"
#include "mem/vmm.h"
#include "term/term.h"
void kprintf_elfsect(Elf64_Shdr* hdr)
{
	kprintf("kprintf_elfsect(): Elf Section Header: type %u\r\n", hdr->sh_type);
}
nyauxsymbol find_from_rip(uint64_t rip)
{
	for (uint64_t i = 0; i != symbolarray->size - 1; i++)
	{
		nyauxsymbol bro = symbolarray->array[i];
		if (bro.function_address <= rip && bro.function_address != 0 &&
			kernel_address.response->virtual_base < bro.function_address && rip <= bro.function_address + bro.function_size)
		{
			return symbolarray->array[i];
		}
	}
	nyauxsymbol h = {.function_address = 0x0, .function_name = "Unknown"};
	return h;
}
nyauxsymbolresource* symbolarray;

static void bubblesort(int length)
{
	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < (length - i - 1); j++)
		{
			if (symbolarray->array[j].function_address < symbolarray->array[j + 1].function_address)
			{
				nyauxsymbol temp = symbolarray->array[j];
				symbolarray->array[j] = symbolarray->array[j + 1];
				symbolarray->array[j + 1] = temp;
			}
		}
	}
}
void get_symbols()
{
	Elf64_Ehdr* hdr = get_kernel_elfheader();
	kprintf("get_symbols(): found elf signature %c%c%c%c\r\n", hdr->e_ident[0], hdr->e_ident[1], hdr->e_ident[2],
			hdr->e_ident[3]);
	Elf64_Shdr* sections = ((Elf64_Shdr*)(get_kerneL_address() + hdr->e_shoff));
	Elf64_Shdr* strtab = NULL;
	Elf64_Shdr* symtab = NULL;
	kprintf_elfsect(sections);

	for (int i = 0; i < hdr->e_shnum; i++)
	{
		Elf64_Shdr* oh = (Elf64_Shdr*)((uint64_t)sections + (hdr->e_shentsize * i));

		if (oh->sh_type == 2)
		{
			symtab = oh;
		}
		if (oh->sh_type == 3 && i != hdr->e_shstrndx)
		{
			strtab = oh;
		}
	}
	if (strtab && symtab)
	{
		char* him = (char*)(get_kerneL_address() + strtab->sh_offset);
		uint64_t amou_of_sym = (symtab->sh_size / symtab->sh_entsize);

		nyauxsymbol* array = (nyauxsymbol*)(kmalloc(sizeof(nyauxsymbol) * amou_of_sym));
		memset(array, 0, sizeof(nyauxsymbol) * amou_of_sym);
		uint64_t count = 0;
		for (uint64_t i = 1; i != amou_of_sym; i++)
		{
			Elf64_Sym* symbo = (Elf64_Sym*)(get_kerneL_address() + symtab->sh_offset + (i * symtab->sh_entsize));
			if (symbo->st_value == 0)
				continue;

			char* nameofsym = (char*)((uint64_t)him + symbo->st_name);
			array[count].function_name = nameofsym;
			array[count].function_size = symbo->st_size;
			array[count].function_address = symbo->st_value;
			count += 1;
		}
		nyauxsymbolresource* h = kmalloc(sizeof(nyauxsymbolresource));
		memset(h, 0, sizeof(nyauxsymbolresource));
		h->size = count;
		h->array = array;
		symbolarray = h;
		bubblesort(h->size);

		kprintf("get_symbols(): Nyaux Symbol Resource Created and Stored on "
				"Heap\r\n");
	}
	else
	{
		panic("get_symbols(): Couldn't Read Symbols");
	}
}
