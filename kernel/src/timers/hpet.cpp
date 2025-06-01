
#include <cstdint>
#include <timers/hpet.hpp>

void HPET::init_timer() {
    if (hpetvirtaddr != NULL)
	{
		return;
	}
	uacpi_table hpet_table;
	uacpi_status st = uacpi_table_find_by_signature("HPET", &hpet_table);
	if (st != UACPI_STATUS_OK)
	{
		panic((char*)"The Nyaux Kernel does not support Devices without a HPET\r\nThe "
			  "Reason for this is becausei hate tsc with a burning desire. pr in a "
			  "different calibration timer if u want one so bad lol");
	}
	kprintf("init_hpet(): Timer Table Found\r\n");
	struct acpi_hpet* hpet = (struct acpi_hpet*)hpet_table.virt_addr;
	kprintf("init_hpet(): HPET has Physical Address 0x%lx\r\n", hpet->address.address);
	volatile uint32_t* cap = (volatile uint32_t*)(hpet->address.address + hhdm_request.response->offset);
	if (!(*cap & (1 << 13)))
	{
		kprintf("init_hpet(): Bit 13 is off\r\ninit_hpet(): Panicking.\r\n");
		bit32 = true;
		volatile uint64_t* capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		ctr_clock_period_fs = ((volatile uint32_t)(*capr >> 32));
		kprintf("init_hpet(): Counter Clock Period in fentoseconds%lu\r\n", ctr_clock_period_fs);
		capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset + 0x10);
		*capr |= 1;	   // enable counter
        kprintf("HPET config after enable write: 0x%lx\r\n", *capr);

		kprintf("init_hpet(): Main Counter Enabled!\r\n");
		hpetvirtaddr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		bit32 = true;
	}
	else
	{
		volatile uint64_t* capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
		ctr_clock_period_fs = ((volatile uint64_t)(*capr >> 32));
		kprintf("init_hpet(): Counter Clock Period %lu\r\n", ctr_clock_period_fs);
		capr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset + 0x10);
		*capr |= 1;	   // enable counter

		kprintf("init_hpet(): Main Counter Enabled!\r\n");
        kprintf("HPET config after enable write: 0x%lx\n", *capr);
		hpetvirtaddr = (volatile uint64_t*)(hpet->address.address + hhdm_request.response->offset);
	}
    
}
size_t HPET::get_fs() {
    if (hpetvirtaddr == NULL) {
        return 0;
    }

    volatile void* counter_reg = (volatile void*)((uintptr_t)hpetvirtaddr + 0xF0);

    if (!bit32) {
        return (*(volatile uint64_t*)counter_reg) * ctr_clock_period_fs;
    } else {
        return (*(volatile uint32_t*)counter_reg) * ctr_clock_period_fs;
    }
}

size_t HPET::get_ns() {
    if (hpetvirtaddr == NULL) {
        return 0;
    }

    volatile void* counter_reg = (volatile void*)((uintptr_t)hpetvirtaddr + 0xF0);

    if (!bit32) {
        return ((*(volatile uint64_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000;
    } else {
        return ((*(volatile uint32_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000;
    }
}

size_t HPET::get_us() {
    if (hpetvirtaddr == NULL) {
        return 0;
    }

    volatile void* counter_reg = (volatile void*)((uintptr_t)hpetvirtaddr + 0xF0);

    if (!bit32) {
        return ((*(volatile uint64_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000'000;
    } else {
        return ((*(volatile uint32_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000'000;
    }
}

size_t HPET::get_ms() {
    if (hpetvirtaddr == NULL) {
        return 0;
    }

    volatile void* counter_reg = (volatile void*)((uintptr_t)hpetvirtaddr + 0xF0);

    if (!bit32) {
        return ((*(volatile uint64_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000'000'000;
    } else {
        return ((*(volatile uint32_t*)counter_reg) * ctr_clock_period_fs) / 1'000'000'000'000;
    }
    
}

HPET::HPET() {
    init_timer();
}
int HPET::stall_poll_ns(size_t ns) {
        
    if (!hpetvirtaddr) {
        return -1;
    }
    
        if (!bit32) {
            volatile uint64_t* hpet_timer = reinterpret_cast<volatile uint64_t*>(hpetvirtaddr + 0xF0);
            uint64_t pol_start = *hpet_timer;

            while ((*hpet_timer - pol_start) * ctr_clock_period_fs < ns * 1000000) {
                
            }
        } else {
            volatile uint32_t* hpet_timer = reinterpret_cast<volatile uint32_t*>(hpetvirtaddr + 0xF0);
            uint32_t pol_start = *hpet_timer;

            while (static_cast<uint64_t>(*hpet_timer - pol_start) * ctr_clock_period_fs < ns * 1000000) {
                
            }
        }

        return 0;
}
int HPET::stall_poll_us(size_t usec) {
    if (!hpetvirtaddr) {
        return -1;
    }
        if (!bit32) {
            volatile uint64_t* hpet_timer = reinterpret_cast<volatile uint64_t*>(hpetvirtaddr + 0xF0);
            uint64_t pol_start = *hpet_timer;

            while ((*hpet_timer - pol_start) * ctr_clock_period_fs < usec * 1000000000) {
                
            }
        } else {
            volatile uint32_t* hpet_timer = reinterpret_cast<volatile uint32_t*>(hpetvirtaddr + 0xF0);
            uint32_t pol_start = *hpet_timer;

            while (static_cast<uint64_t>(*hpet_timer - pol_start) * ctr_clock_period_fs < usec * 1000000000) {
                
            }
        }

        return 0;
    }
    int HPET::stall_poll_ms(size_t ms) {
        if (!hpetvirtaddr) {
        return -1;
    }
        const uint64_t hpet_offset = 0xF0;

        if (!bit32) {
            volatile uint64_t* hpet_timer = reinterpret_cast<volatile uint64_t*>((uint64_t)hpetvirtaddr + hpet_offset);
            uint64_t pol_start = *hpet_timer;

            while ((*hpet_timer - pol_start) * ctr_clock_period_fs < ms * 1000000000000) {

            

            }
        } else {
            volatile uint32_t* hpet_timer = reinterpret_cast<volatile uint32_t*>(hpetvirtaddr + hpet_offset);
            uint32_t pol_start = *hpet_timer;

            while (static_cast<uint64_t>(*hpet_timer - pol_start) * ctr_clock_period_fs < ms * 1000000000000) {
                
            }
        }
    return 0;
}
    size_t HPET::get_ps() {
    return 0;
}

int HPET::stall_poll_ps(size_t) {
    return -1;
}

int HPET::stall_poll_fs(size_t) {
    return -1;
}