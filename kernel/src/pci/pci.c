#include "pci.h"

#include <stdint.h>

#include "arch/arch.h"

void pciconfigselect(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	uint32_t address = ((uint32_t)1 << 31) | (((uint32_t)bus) << 16) | (((uint32_t)device) << 11) |
					   (((uint32_t)func) << 8) | (((uint32_t)offset) & 0xFC);
	arch_raw_io_write(CONFIG_ADDRESS_REG, address, 4);
}
uint8_t pciconfigreadbyte(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	pciconfigselect(bus, device, func, offset);
	uint32_t value = arch_raw_io_in(CONFIG_DATA_REG, 4);
	uint8_t tmp = (uint16_t)((value) >> ((offset & 3) * 8)) & 0xFF;
	return tmp;
}
uint16_t pciconfigreadword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	pciconfigselect(bus, device, func, offset);
	uint32_t value = arch_raw_io_in(CONFIG_DATA_REG, 4);
	uint16_t tmp = (uint16_t)(((value) >> ((offset & 2) * 8)) & 0xFFFF);

	return tmp;
}
uint32_t pciconfigread32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	pciconfigselect(bus, device, func, offset);
	uint32_t value = arch_raw_io_in(CONFIG_DATA_REG, 4);
	return value;
}
void pciconfigwritebyte(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t valuer)
{
	pciconfigselect(bus, device, func, offset);
	uint32_t value = arch_raw_io_in(CONFIG_DATA_REG, 4);
	value &= ~(0xff << ((offset & 3) * 8));
	value |= (valuer << ((offset & 3) * 8));
	arch_raw_io_write(CONFIG_DATA_REG, value, 4);
}
void pciconfigwriteword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t valuer)
{
	pciconfigselect(bus, device, func, offset);
	uint32_t value = arch_raw_io_in(CONFIG_DATA_REG, 4);
	value &= ~(0xffff << ((offset & 2) * 8));
	value |= (valuer << ((offset & 2) * 8));
	arch_raw_io_write(CONFIG_DATA_REG, value, 4);
}
void pciconfigwrite32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t valuer)
{
	pciconfigselect(bus, device, func, offset);
	arch_raw_io_write(CONFIG_DATA_REG, valuer, 4);
}
