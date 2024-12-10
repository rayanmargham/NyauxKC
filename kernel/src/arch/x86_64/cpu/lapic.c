#include "lapic.h"
#include "../instructions/instructions.h"
#include <mem/pmm.h>
#include <stdint.h>
#include <term/term.h>
#include <timers/hpet.h>
// per cpu function call
void init_lapic() {
  kprintf("CPU %d's lapic is being initialized.\n", get_lapic_id());
  volatile uint64_t lapic = get_lapic_address() + hhdm_request.response->offset;
  volatile uint32_t *sur_interrupt = (volatile uint32_t *)(lapic + 0xf0);
  *sur_interrupt = 34 | (*sur_interrupt & ~0xFF) |
                   (1 << 8); // removing the 0-7 bits in the register so
                             // i can put my vector in js fine
  volatile uint32_t *lapic_timer_divide_configuration_register =
      (volatile uint32_t *)(lapic + 0x3e0);
  *lapic_timer_divide_configuration_register = 1;
  volatile uint32_t *lapic_config = (volatile uint32_t *)(lapic + 0x320);
  *lapic_config = 32 | (1 << 16);
  volatile uint32_t *lapic_inital_count = (volatile uint32_t *)(lapic + 0x380);
  volatile uint32_t *lapic_cur_count = (volatile uint32_t *)(lapic + 0x390);
  *lapic_inital_count = 0xffffffff;
  stall_with_hpetclk(10);
  *lapic_inital_count = 0xffffffff - *lapic_cur_count;
  *lapic_config = 32 | (0 << 16) | (1 << 17);
  __asm__("sti");
}
void send_eoi() {
  volatile uint64_t lapic = get_lapic_address() + hhdm_request.response->offset;
  volatile uint32_t *eoi_reg = (volatile uint32_t *)(lapic + 0xb0);
  *eoi_reg = 0;
}
