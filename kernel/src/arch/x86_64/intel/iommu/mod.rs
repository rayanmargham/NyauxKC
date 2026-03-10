use alloc::vec::Vec;
use nyaux_uacpi_bindings::{uacpi_table, uacpi_table_find_by_signature};

use crate::{arch::{Arch, Processor}, early_init_pagemap, memory::vmm::VMMFlags, println, uacpi::check_ustatus};
#[repr(C, packed)]
struct root_table_entry(u128);

impl root_table_entry {
    fn new(ctp: u64) -> root_table_entry {
        let a = (ctp| 1) as u128;
        return root_table_entry(a);
    }
}
#[repr(C, packed)]
struct context_table_entry(u128);

#[repr(C, packed)]
struct acpi_drhd {
    typ: u16,
    length: u16,
    flags: u8,
    size: u8,
    segnum: u16,
    reg_base_addr: u64,

}

#[repr(C, packed)]
struct acpi_dmar {
    signature: [u8; 4],
    length: u32,
    revision: u8,
    checksum: u8,
    oem_id: [u8; 6],
    oem_table_id: [u8; 8],
    oem_revision: u32,
    creator_id: u32,
    creator_revision: u32,
    host_address_width: u8,
    flags: u8,
    reserved: [u8; 10],

}

pub fn iommu_init() {
    println!("init");
    let mut table: uacpi_table = unsafe {core::mem::zeroed()};
        let status = unsafe { uacpi_table_find_by_signature(c"DMAR".as_ptr(), &mut table) };
        check_ustatus(status).unwrap();

        let dmar = unsafe {table.__bindgen_anon_1.virt_addr as *const acpi_dmar};
        let mut cur = unsafe{dmar.add(1).cast::<u8>()};
        let end = unsafe {dmar.cast::<u8>().add(dmar.read().length as usize)};
        let mut iommu_addr = 0;
        while cur != end {
            let hd = cur.cast::<acpi_drhd>();
            if unsafe {(*hd).typ == 0} {
                iommu_addr = unsafe {(*hd).reg_base_addr};
            }
            cur = unsafe {cur.byte_add((*hd).length as usize)};
        }
        println!("found iommu with address 0x{:x}", iommu_addr);
        let mut ph = Vec::new();
        for i in (iommu_addr..(iommu_addr + Processor::PAGE_SIZE as u64)).step_by(Processor::PAGE_SIZE) {
            ph.push(i);
        }
        let iommu_virt = early_init_pagemap!().vmm_alloc_with_phys(Processor::PAGE_SIZE, VMMFlags::NOCACHE | VMMFlags::WRITE | VMMFlags::GLOBAL, ph).unwrap();
        let version = unsafe {iommu_virt.cast::<u32>().read_volatile()};
        let major_ver = ((version >> 4) & 0xF) as u8;
        let minor_ver = (version & 0xF) as u8;
        let cap = unsafe {iommu_virt.cast::<u64>().add(1).read_volatile()};
        println!("iommu version major {} minor {}", major_ver, minor_ver);
        
        println!("capablities {:b}", cap);
        if cap & (1 << 10) == 0 {
            println!("nyaux does not support iommus without 4 level paging mb twin");
            return;
        }



}