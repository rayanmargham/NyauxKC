
#include "cmos.h"

#include <stdint.h>

#include "arch/arch.h"
#define CMOS_SELECT 0x70
#define CMOS_RW		0x71
uint8_t read_cmos_reg(volatile uint8_t reg)
{
	// disable nmi because fuck you
	arch_raw_io_write(CMOS_SELECT, (1 << 7) | (reg), 1);
	return arch_raw_io_in(CMOS_RW, 1);
}
void write_cmos_reg(volatile uint8_t reg, volatile uint8_t data)
{
	arch_raw_io_write(CMOS_SELECT, (1 << 7) | (reg), 1);
	arch_raw_io_write(CMOS_RW, (uint64_t)data, 1);
}
void get_time()
{
	write_cmos_reg(0x0B,
				   (1 << 2) | (1 << 1));	// read https://bochs.sourceforge.io/techspec/CMOS-reference.txt for spec
											// about what im doing here
	volatile uint8_t val = read_cmos_reg(0x32);	   // century register
	volatile uint8_t val2 = read_cmos_reg(0x09);
	volatile uint8_t val3 = read_cmos_reg(0x08);
	volatile uint8_t val4 = read_cmos_reg(0x07);
	volatile uint8_t val5 = read_cmos_reg(0x04);
	volatile uint8_t val6 = read_cmos_reg(0x02);
	volatile uint8_t val7 = read_cmos_reg(0x00);
	kprintf_log(TRACE, "get_time(): [%d%d/%d/%d--%d:%d:%d]\n", val, val2, val3, val4, val5, val6, val7);
}
