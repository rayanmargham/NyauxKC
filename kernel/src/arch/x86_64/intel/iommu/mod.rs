use alloc::vec::Vec;
use nyaux_uacpi_bindings::{uacpi_table, uacpi_table_find_by_signature};

use crate::{
    HHDM_REQUEST,
    arch::{Arch, Processor},
    early_init_pagemap,
    memory::{pmm::allocate_page, vmm::VMMFlags},
    pci::pci_devices,
    println,
    uacpi::check_ustatus,
    util::{Once, find_acpi_table},
};
#[repr(C, packed)]
struct root_table {
    entries: [u128; 256],
}

impl root_table {
    fn new() -> &'static mut root_table {
        let root_table = allocate_page().cast::<root_table>();
        unsafe { root_table.as_mut().unwrap() }
    }
}
#[repr(C, packed)]
struct context_table {
    entries: [u128; 256]
}

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
struct iommu {
    regs_base: *mut u8,
    root_table: *mut root_table,
}
unsafe impl Send for iommu {} // RUST please be quiet
unsafe impl Sync for iommu {}
static root_table_gb: Once<iommu> = Once::new();

pub fn iommu_init() {
    println!("init");

    let table = find_acpi_table(c"DMAR".as_ptr()).unwrap();
    let dmar = unsafe { table.__bindgen_anon_1.virt_addr as *const acpi_dmar };
    let mut cur = unsafe { dmar.add(1).cast::<u8>() };
    let end = unsafe { dmar.cast::<u8>().add(dmar.read().length as usize) };
    let mut iommu_addr = 0;
    while cur != end {
        let hd = cur.cast::<acpi_drhd>();
        if unsafe { (*hd).typ == 0 } {
            iommu_addr = unsafe { (*hd).reg_base_addr };
        }
        cur = unsafe { cur.byte_add((*hd).length as usize) };
    }
    println!("found iommu with address 0x{:x}", iommu_addr);
    let mut ph = Vec::new();
    for i in (iommu_addr..(iommu_addr + Processor::PAGE_SIZE as u64)).step_by(Processor::PAGE_SIZE)
    {
        ph.push(i);
    }
    let iommu_virt = early_init_pagemap!()
        .vmm_alloc_with_phys(
            Processor::PAGE_SIZE,
            VMMFlags::NOCACHE | VMMFlags::WRITE | VMMFlags::GLOBAL,
            ph,
        )
        .unwrap();
    let version = unsafe { iommu_virt.cast::<u32>().read_volatile() };
    let major_ver = ((version >> 4) & 0xF) as u8;
    let minor_ver = (version & 0xF) as u8;
    let cap = unsafe { iommu_virt.cast::<u64>().add(1).read_volatile() };
    println!("iommu version major {} minor {}", major_ver, minor_ver);

    println!("capablities {:b}", cap);
    if cap & (1 << 10) == 0 {
        println!("nyaux does not support iommus without 4 level paging mb twin");
        return;
    }
    let ecap = unsafe {
        iommu_virt.cast::<u64>()
            .add(2).read_volatile()
    };
    if ecap & (1 << 6) == 0 {
        println!("i need passthrough mode twin");
        return;
    }
    let rtaddr_reg = unsafe { iommu_virt.cast::<u64>().add(4) };
    let root = root_table::new();
    unsafe {
        rtaddr_reg.write_volatile(
            (root as *mut root_table).byte_sub(HHDM_REQUEST.response().unwrap().offset as usize)
                as u64,
        )
    };
    root_table_gb.call_once(|| iommu {
        regs_base: iommu_virt.cast(),
        root_table: root as *mut root_table,
    });
    let d = pci_devices.get().unwrap();
    for i in d.iter() {
        let context_table = {
            if root.entries[i.0 as usize] == 0 {
                let new_context_table = allocate_page();
                root.entries[i.0 as usize] = unsafe {
                    new_context_table
                        .byte_sub(HHDM_REQUEST.response().unwrap().offset as usize)
                        .addr()
                        | 1
                } as u128;
                unsafe {new_context_table.cast::<context_table>().as_mut().unwrap()}
            } else {
                unsafe {core::ptr::with_exposed_provenance_mut::<context_table>(
                    ((root.entries[i.0 as usize] & !0x1) as usize)
                        + HHDM_REQUEST.response().unwrap().offset as usize,
                ).as_mut().unwrap()}
            }
        };
        // pick domain 1 for now, passthrough
        let context_val: u128 = (1u128 << 72) | (0b10u128 << 64) | (0b10 << 2) | 1;
        let ctx_idx = (i.1 << 3) | (i.2);
        context_table.entries[ctx_idx as usize] = context_val;
    }
    println!("context tables in passthrough mode");
    let gc = unsafe {iommu_virt.cast::<u64>().add(3).cast::<u32>()};
    let gs = unsafe {
        gc
            .add(1)
    };
    let mut status = unsafe {
        gs.read_volatile() & 0x96FFFFFF // so basically, spec tells me to do this cause
        // to get rid of the one shot bits
    };
    status = status | (1 << 30);
    unsafe {
        gc.write_volatile(status);
    }
    while (unsafe {
        gs.read_volatile() & (1 << 30)
    } == 0) {

    }
    status = unsafe {
        gs.read_volatile() & 0x96FFFFFF 
    };
    status = status | (1 << 31);
    unsafe {gc.write_volatile(status)};
    while (unsafe {
        gs.read_volatile() & (1 << 31)
    } == 0) {

    }
    println!("all done, hardware said okay to my tables");
    // TODO: actual page tables for the context entries
}
