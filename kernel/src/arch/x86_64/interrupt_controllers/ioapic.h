#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void populate_ioapic();
void route_irq(uint8_t irq, uint8_t vec, uint16_t flags, uint32_t lapic_id);
#ifdef __cplusplus
}
#endif