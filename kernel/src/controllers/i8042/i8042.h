#pragma once
#include <arch/arch.h>
#include <cppglue/glue.hpp>
#include <mem/kmem.h>
#include <term/term.h>
#include <utils/basic.h>
static spinlock_t ps2lock = SPINLOCK_INITIALIZER;
static inline uint8_t read_status_reg() {
  return (uint8_t)(arch_raw_io_in(0x64, 1));
};


static inline void write_data_reg(uint8_t data) {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1 << 1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("WE FUCKING TIMED OUT\r\n");
        spinlock_unlock(&ps2lock);
        
        
    }
  arch_raw_io_write(0x60, (uint8_t)(data), 1);
  spinlock_unlock(&ps2lock);
};

static inline uint8_t read_data_reg() {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("timeout\r\n");
        spinlock_unlock(&ps2lock);
        return 0xff;
        
    }
    uint8_t data = (uint8_t)(arch_raw_io_in(0x60, 1));
  spinlock_unlock(&ps2lock);
  return data;
};
static inline void send_command(uint8_t data) {
    spinlock_lock(&ps2lock);
    size_t i = 0;
    uint8_t prevstatus = 0;
do {
    i++;
    prevstatus = read_status_reg();
} while (i < 0x2000 && (prevstatus & (1 << 1)));
    if (prevstatus & (1 << 1)) {
         // get rid of any shit we dont care abt
         // yes we should error out here but frankly who cares
         kprintf("hmm with data %x\r\n", data);
        spinlock_unlock(&ps2lock);
        
    }
  arch_raw_io_write(0x64, (uint8_t)(data), 1);
  spinlock_unlock(&ps2lock);
};
#ifdef __cplusplus
extern "C" {
#endif
void i8042_init(); 
#ifdef __cplusplus
}
#endif
static char *kbd_pnpids[26] = {"PNP0300",
    "PNP0301",
    "PNP0302",
    "PNP0303",
    "PNP0304",
    "PNP0305",
    "PNP0306",
    "PNP0307",
    "PNP0308",
    "PNP0309",
    "PNP030A",
    "PNP030B",
    "PNP0320",
    "PNP0321",
    "PNP0322",
    "PNP0323",
    "PNP0324",
    "PNP0325",
    "PNP0326",
    "PNP0327",
    "PNP0340",
    "PNP0341",
    "PNP0342",
    "PNP0343",
    "PNP0343",
    "PNP0344",};


enum PS2DEVICETYPE {
    KEYBOARD,
    MOUSE,
    UNKNOWN
};
struct PS2PORT {
    enum PS2DEVICETYPE type;
    bool usable;
};
struct ps2controller {
    bool inited;
    struct PS2PORT port1;
    struct PS2PORT port2;
};