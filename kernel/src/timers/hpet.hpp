#pragma once
#include <timers/timer.hpp>
#include <stddef.h>
#include <term/term.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <utils/basic.h>
#include "uacpi/acpi.h"
#include "uacpi/status.h"
class HPET : public GenericTimer {
    public: 
    bool bit32 = false;
    volatile uint64_t* hpetvirtaddr = NULL;
    uint64_t ctr_clock_period_fs = 0;	  // in fento seconds
    size_t get_ps() override;
    size_t get_fs() override;
    size_t get_ns() override;
    size_t get_us() override;
    size_t get_ms() override;
    int stall_poll_ps(size_t) override;
    int stall_poll_fs(size_t) override; 
    int stall_poll_ns(size_t ns) override;
    int stall_poll_us(size_t us) override;
    int stall_poll_ms(size_t ms) override;

    void init_timer() override;
    HPET();
};