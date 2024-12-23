#include "gdt.h"

#include <stdint.h>

typedef struct
{
	uint16_t size;
	uint64_t offset;
} __attribute__((packed)) GDTR;
uint64_t gdt[11];
extern void gdt_flush(void*);
GDTR gptr = {};
struct TSS
{
	uint32_t reversed;
	uint64_t rsp[3];	// rsp 0, rsp 1, rsp 2
	uint64_t NOTALLOWED;
	uint64_t ists[7];
	uint64_t NONO;
	uint16_t NONONO;
	uint16_t IOPB;
} __attribute__((packed));
struct TSS tss = {0};
void init_gdt()
{
	gdt[0] = 0x0;
	gdt[1] = 0x00009a000000ffff;
	gdt[2] = 0x000093000000ffff;
	gdt[3] = 0x00cf9a000000ffff;
	gdt[4] = 0x00cf93000000ffff;
	gdt[5] = 0x00af9b000000ffff;
	gdt[6] = 0x00af93000000ffff;
	gdt[7] = 0x00aff3000000ffff;
	gdt[8] = 0x00affb000000ffff;
	uint64_t tssaddress = (uint64_t)&tss;
	uint16_t base_low = tssaddress & 0xFFFF;
	uint8_t base_mid = (tssaddress >> 16) & 0xFF;
	uint8_t base_midhi = (tssaddress >> 24) & 0xFF;
	uint32_t base_hi = (tssaddress >> 32) & 0xFFFFFFFF;
	uint8_t access_byte = 0x89;	   // stolen from osdev wiki cause im lazy to construct my own
								   // basically what it means is it describes the tss as avabile and stuff
	uint16_t sizeoftss = sizeof(struct TSS) - 1;
	uint64_t constructedlow =
		((uint64_t)base_low << 16) | ((uint64_t)base_mid << 32) | ((uint64_t)base_midhi << 56) | access_byte;

	uint64_t constructedhi = (uint64_t)base_hi;
	kprintf("constructed low in binary %lb\n", constructedlow);
	kprintf("constructed hi in binary %lb\n", constructedhi);
	gdt[9] = constructedlow;
	gdt[10] = constructedhi;
	gptr.offset = (uint64_t)&gdt;
	gptr.size = sizeof(gdt) - 1;

	gdt_flush(&gptr);
	__builtin_trap();
}
