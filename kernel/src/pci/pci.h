#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/basic.h>

#define CONFIG_ADDRESS_REG 0xCF8
#define CONFIG_DATA_REG 0xCFC
void pciconfigwrite32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset,
                      uint32_t valuer);
void pciconfigwriteword(uint8_t bus, uint8_t device, uint8_t func,
                        uint8_t offset, uint16_t valuer);
void pciconfigwritebyte(uint8_t bus, uint8_t device, uint8_t func,
                        uint8_t offset, uint8_t valuer);
uint32_t pciconfigread32(uint8_t bus, uint8_t device, uint8_t func,
                         uint8_t offset);
uint16_t pciconfigreadword(uint8_t bus, uint8_t device, uint8_t func,
                           uint8_t offset);
uint8_t pciconfigreadbyte(uint8_t bus, uint8_t device, uint8_t func,
                          uint8_t offset);
#ifdef __cplusplus
}
#endif