// ipis, xapic by water bottle
// x2apic by me
use core::hint::spin_loop;

use alloc::vec;

use crate::{
    arch::x86_64::{CalibrationTimer, cpuid, rdmsr, wrmsr},
    early_init_pagemap, get_cpu_local,
    memory::vmm::VMMFlags,
    println,
    util::to_hhdm,
};

const IA32_APIC_BASE_MSR: u32 = 0x1B;
const IA32_X2APIC_BASE_MSR: u32 = 0x800;

const IA32_APIC_BASE_BSP: usize = (1 << 8);
const IA32_APIC_BASE_ENABLE: usize = (1 << 11);
const IA32_APIC_BASE_ENABLE_X2APIC: usize = (1 << 10);

const IA32_CPUID_FEATURE_X2APIC: u32 = (1 << 21);

const LAPIC_ICR_LOW: u32 = 0x300;
const LAPIC_ICR_HIGH: u32 = 0x310;
const LAPIC_X2APIC_ICR: u32 = 0x300;

const IA32_LAPIC_LVT_TIMER: u32 = 0x320;
const IA32_LAPIC_LVT_THERMAL: u32 = 0x330;
const IA32_LAPIC_LVT_PERF: u32 = 0x340;
const IA32_LAPIC_LVT_LINT0: u32 = 0x350;
const IA32_LAPIC_LVT_LINT1: u32 = 0x360;
const IA32_LAPIC_LVT_ERROR: u32 = 0x370;

const IA32_LAPIC_TIMER_INIT_COUNT: u32 = 0x380;
const IA32_LAPIC_TIMER_CUR_COUNT: u32 = 0x390;
const IA32_LAPIC_TIMER_DIV: u32 = 0x3E0;

const IA32_LAPIC_ID: u32 = 0x20;
const IA32_LAPIC_TPR: u32 = 0x80;
const IA32_LAPIC_EOI: u32 = 0xB0;
const IA32_LAPIC_LDR: u32 = 0xD0;
const IA32_LAPIC_DFR: u32 = 0xE0;
const IA32_LAPIC_SPURIOUS: u32 = 0xF0;

const IA32_LAPIC_MASK_MASKED: u32 = 0x10000;

const IA32_LAPIC_SHORTHAND_NONE: u32 = 0x00000;
const IA32_LAPIC_SHORTHAND_SELF: u32 = 0x40000;
const IA32_LAPIC_SHORTHAND_ALL_INCL_SELF: u32 = 0x80000;
const IA32_LAPIC_SHORTHAND_ALL_EXCL_SELF: u32 = 0xC0000;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum lapic_type {
    xapic,
    x2apic,
}

pub struct lapic {
    lapic_addr: *mut u64,
    lapic_id: u32,
    ticks_per_ms: usize,
    bsp: bool,
    lapic_type: lapic_type,
}

impl lapic {
    fn new() -> Self {
        let base_msr = rdmsr(IA32_APIC_BASE_MSR);
        let mut is_bsp = false;
        if (base_msr & IA32_APIC_BASE_BSP) > 0 {
            is_bsp = true;
        }

        let lapic_phys_addr: u64 = (base_msr & 0xFFFFFF000) as u64;
        let addr = to_hhdm(lapic_phys_addr as *mut u64);
        early_init_pagemap!().arch_map_region(
            addr as usize,
            4096,
            vec![lapic_phys_addr],
            VMMFlags::NOCACHE | VMMFlags::GLOBAL | VMMFlags::WRITE,
        );

        let mut data = lapic {
            lapic_addr: addr,
            lapic_id: 0,
            bsp: is_bsp,
            ticks_per_ms: 0,
            lapic_type: lapic_type::xapic,
        };

        data.lapic_type = data
            .configure_lapic_type()
            .expect("failed to configure lapic type");

        data.lapic_id = match data.lapic_type {
            lapic_type::xapic => (data.read(IA32_LAPIC_ID) >> 24) & 0xff,
            lapic_type::x2apic => data.read(IA32_LAPIC_ID),
        };

        data.write(IA32_LAPIC_TPR, 0);
        if (data.lapic_type == lapic_type::xapic) {
            data.write(IA32_LAPIC_DFR, 0xF0000000);
            data.write(IA32_LAPIC_LDR, 0x01000000);
        }

        data.write(IA32_LAPIC_LVT_TIMER, IA32_LAPIC_MASK_MASKED);
        data.write(IA32_LAPIC_LVT_THERMAL, IA32_LAPIC_MASK_MASKED);
        data.write(IA32_LAPIC_LVT_PERF, IA32_LAPIC_MASK_MASKED);
        data.write(IA32_LAPIC_LVT_LINT0, IA32_LAPIC_MASK_MASKED);
        data.write(IA32_LAPIC_LVT_LINT1, IA32_LAPIC_MASK_MASKED);
        data.write(IA32_LAPIC_LVT_ERROR, IA32_LAPIC_MASK_MASKED);

        data.write(IA32_LAPIC_SPURIOUS, (0xFF) | (1 << 8));

        return data;
    }

    fn read(&self, reg: u32) -> u32 {
        match self.lapic_type {
            lapic_type::xapic => unsafe {
                self.lapic_addr
                    .byte_add(reg as usize)
                    .cast::<u32>()
                    .read_volatile()
            },
            lapic_type::x2apic => rdmsr(IA32_X2APIC_BASE_MSR + (reg >> 4)) as u32,
        }
    }

    fn write(&mut self, reg: u32, val: u32) {
        match self.lapic_type {
            lapic_type::xapic => unsafe {
                self.lapic_addr
                    .byte_add(reg as usize)
                    .cast::<u32>()
                    .write_volatile(val)
            },
            lapic_type::x2apic => wrmsr(IA32_X2APIC_BASE_MSR + (reg >> 4), val as usize),
        }
    }

    fn read_64(&self, reg: u32) -> u64 {
        match self.lapic_type {
            lapic_type::xapic => panic!("xapic does not support 64 bit reads"),
            lapic_type::x2apic => rdmsr(IA32_X2APIC_BASE_MSR + (reg >> 4)) as u64,
        }
    }

    fn write_64(&mut self, reg: u32, val: u64) {
        match self.lapic_type {
            lapic_type::xapic => panic!("xapic does not support 64 bit writes"),
            lapic_type::x2apic => wrmsr(IA32_X2APIC_BASE_MSR + (reg >> 4), val as usize),
        }
    }

    fn calibrate_timer(&mut self, cali: &dyn CalibrationTimer) {
        // timer on 0x21, masked
        self.write(IA32_LAPIC_LVT_TIMER, 0x21 | (1 << 16));

        self.write(IA32_LAPIC_TIMER_DIV, 0xb); // divide by 1
        self.write(IA32_LAPIC_TIMER_INIT_COUNT, 0xFFFFFFFF);
        cali.poll_for_ms(10);
        let cur = self.read(IA32_LAPIC_TIMER_CUR_COUNT);
        self.ticks_per_ms = ((0xFFFFFFFF - cur) / 10) as usize;
        self.write(IA32_LAPIC_TIMER_INIT_COUNT, 0);
        println!(
            "calibrated lapic timer, ticks per 1ms {}",
            self.ticks_per_ms
        );
    }
    pub fn disable_timer(&mut self) {
        let mut lvt = self.read(IA32_LAPIC_LVT_TIMER);
        lvt |= IA32_LAPIC_MASK_MASKED;

        self.write(IA32_LAPIC_LVT_TIMER, lvt);
    }
    pub fn enable_timer(&mut self) {
        let mut lvt = self.read(IA32_LAPIC_LVT_TIMER);
        lvt &= !IA32_LAPIC_MASK_MASKED;

        self.write(IA32_LAPIC_LVT_TIMER, lvt); 
    }
    pub fn set_timer(&mut self, ms: usize) {

        self.disable_timer();
        self.write(IA32_LAPIC_TIMER_INIT_COUNT, (self.ticks_per_ms * ms) as u32);
        self.enable_timer();
        // your on your own kiddo
    }

    fn configure_lapic_type(&mut self) -> Result<(lapic_type), &'static str> {
        let mut apic_config = rdmsr(IA32_APIC_BASE_MSR);

        if !((apic_config & IA32_APIC_BASE_ENABLE) > 0) {
            apic_config |= IA32_APIC_BASE_ENABLE;
            wrmsr(IA32_APIC_BASE_MSR, apic_config);
            apic_config = rdmsr(IA32_APIC_BASE_MSR);
        }

        let features = cpuid(1, 0);
        if ((features.2 & IA32_CPUID_FEATURE_X2APIC) > 0) {
            if !((apic_config & IA32_APIC_BASE_ENABLE_X2APIC) > 0) {
                apic_config |= IA32_APIC_BASE_ENABLE_X2APIC;
                wrmsr(IA32_APIC_BASE_MSR, apic_config);
                apic_config = rdmsr(IA32_APIC_BASE_MSR);
            }
            println!("entered x2apic mode");
            return (Ok((lapic_type::x2apic)));
        } else {
            println!("entered xapic mode");
            return Ok((lapic_type::xapic));
        }
    }

    pub fn send_eoi(&mut self) {
        self.write(IA32_LAPIC_EOI, 0);
    }

    fn send_icr(&mut self, apic_id: u32, mut icr: u64) {

        if self.lapic_type == lapic_type::xapic {
            // for xapic, the destination field is bits 63-56
            icr |= (apic_id as u64) << 56;
        } else {
            // for x2apic, the destination field is bits 63-32
            icr |= (apic_id as u64) << 32;
        }

        if (self.lapic_type == lapic_type::x2apic) {
            self.write_64(LAPIC_X2APIC_ICR, icr);
        } else {
            self.write(LAPIC_ICR_HIGH, (icr >> 32) as u32);
            // @note: watch out if you are interrupted right here
            // and you try to send an ipi from that interrupt
            // you will corrupt the icr high value and when you return from the interrupt and write the low value you'll send a garbage ipi
            self.write(LAPIC_ICR_LOW, (icr & 0xFFFFFFFF) as u32);
            while ((self.read(LAPIC_ICR_LOW) as u64) & (1 << 12)) > 0 {
                core::hint::spin_loop();
            }
        }
    }

    fn send_ipi(&mut self, apic_id: u32, vector: u8) {
        let mut icr: u64 = 0;

        icr |= IA32_LAPIC_SHORTHAND_NONE as u64;
        icr |= vector as u64;

        self.send_icr(apic_id, icr);
    }

    fn broadcast_ipi(&mut self, apic_id: u32, vector: u8) {
        let mut icr: u64 = 0;

        icr |= IA32_LAPIC_SHORTHAND_ALL_EXCL_SELF as u64;
        icr |= vector as u64;

        self.send_icr(0, icr);
    }
}

pub fn lapic_init(cali: &dyn CalibrationTimer) {
    let mut l = lapic::new();
    l.calibrate_timer(cali);
    let ll = unsafe { get_cpu_local!().as_mut().unwrap() };
    ll.lapic = Some(l);
}
