use alloc::vec;
use nyaux_uacpi_bindings::acpi_hpet;

use crate::{HHDM_REQUEST, early_init_pagemap, memory::vmm::VMMFlags, println, status, util::{Once, find_acpi_table}};
use super::CalibrationTimer;


pub struct hpet {
    virt_addr: *mut u64,
    morebitcounter: bool,
    ctr_clk_period: usize
}
unsafe impl Send for hpet {}
unsafe impl Sync for hpet {}


impl hpet {
    fn new(hpet_table: acpi_hpet) -> Self {
        let addr = hpet_table.address.address;
        let virt_addr = addr + HHDM_REQUEST.response().unwrap().offset;
        early_init_pagemap!().arch_map_region(virt_addr as usize, 4096, vec![addr], VMMFlags::NOCACHE | VMMFlags::GLOBAL | VMMFlags::WRITE);
        let virt_ptr = core::ptr::without_provenance_mut::<u64>(virt_addr as usize);
        let mut hpe = hpet {
            virt_addr: virt_ptr,
            morebitcounter: false,
            ctr_clk_period: 0
        };
        let cap = hpe.read_reg_u64(0);
        let rev_id = cap & 0xFF;
        let hm = (cap >> 13) & 0x1;
        if hm == 1 {

            hpe.morebitcounter = true;
        }
        let ctr_clock = cap >> 32 & 0xFFFFFFFF;
        hpe.ctr_clk_period = ctr_clock as usize;
        let mut gcr = hpe.read_reg_u64(0x10);
        gcr |= 1;
        hpe.write_reg_u64(0x10, gcr);
        println!("hpet init revision {}, 64 bit or 32 bit counter {}, counter enabled!", rev_id, hm);
        return hpe;
    }
    fn read_reg_u64(&self, byte_offset: usize) -> u64 {
        let reg = unsafe {self.virt_addr.byte_add(byte_offset)};
        let val = unsafe {reg.read_volatile()};
        return val;
    }
    fn write_reg_u64(&self, byte_offset: usize, val: u64){
        let reg = unsafe {self.virt_addr.byte_add(byte_offset)};
        unsafe {reg.write_volatile(val)};
    }
    fn read_reg_u32(&self, byte_offset: usize) -> u32 {
        let reg = unsafe {self.virt_addr.byte_add(byte_offset).cast::<u32>()};
        let val = unsafe {reg.read_volatile()};
        return val;
    }
    fn write_reg_u32(&self, byte_offset: usize, val: u32){
        let reg = unsafe {self.virt_addr.byte_add(byte_offset).cast::<u32>()};
        unsafe {reg.write_volatile(val)};
    }
    fn read_main_counter(&self) -> usize {
        if !self.morebitcounter {
            return self.read_reg_u32(0x0f0) as usize;
        } else {
            return self.read_reg_u64(0x0f0) as usize;
        }
    }
}

impl CalibrationTimer for hpet {
    fn get_ms(&self) -> usize {
        ((self.read_main_counter() as u128 * self.ctr_clk_period as u128) / 1_000_000_000_000) as usize
    }

    fn get_ns(&self) -> usize {
        ((self.read_main_counter() as u128 * self.ctr_clk_period as u128) / 1_000_000) as usize
    }
    fn poll_for_ms(&self, ms: usize) {
        let inital = self.get_ms();
        while self.get_ms() - inital < ms {
            core::hint::spin_loop();
        }
    }
}

pub fn hpet_init() -> Result<hpet, &'static str> {
    let mut tabl = find_acpi_table(c"HPET".as_ptr());
    if tabl.is_err() {
        return Err(tabl.err().unwrap());
    }
    let the_table = unsafe {tabl.unwrap().__bindgen_anon_1.virt_addr} as *const acpi_hpet;
    let new = hpet::new(unsafe {the_table.read_volatile()});
    Ok(new)
}