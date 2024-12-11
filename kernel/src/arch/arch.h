#pragma once
#include "uacpi/types.h"
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>
void raw_io_write(uint64_t address, uint64_t data, uint8_t byte_width);
uint64_t raw_io_in(uint64_t address, uint8_t byte_width);
int uacpi_arch_install_irq(uacpi_interrupt_handler handler, uacpi_handle ctx,
                           uacpi_handle *out_irq_handle);
void arch_init();
void arch_late_init();
