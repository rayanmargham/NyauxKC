use core::fmt::Debug;

use nyaux_uacpi_bindings::{ACPI_MCFG_SIGNATURE, UACPI_STATUS_OK, acpi_mcfg, uacpi_table, uacpi_table_find_by_signature, uacpi_table_unref};

use crate::{arch::{Arch, Processor}, println, util::Once};

const PCI_CONFIG_ADDR: usize = 0xCF8;
const PCI_DATA: usize = 0xCFC;


pub fn pci_read_dword(bus: u8, slot: u8, func: u8, offset: u8) -> u32 {
    let addr = ((bus as u32) << 16) | ((slot as u32) << 11) | ((func as u32) << 8) | (offset & 0xFC) as u32 | 0x80000000;
    Processor::raw_io_out(PCI_CONFIG_ADDR as u64, addr as u64, 4);
    return Processor::raw_io_in(PCI_DATA as u64, 4) as u32;
}
pub fn pci_write_dword(bus: u8, slot: u8, func: u8, offset: u8, data: u32) {
    let addr = ((bus as u32) << 16) | ((slot as u32) << 11) | ((func as u32) << 8) | (offset & 0xFC) as u32 | 0x80000000;
    Processor::raw_io_out(PCI_CONFIG_ADDR as u64, addr as u64, 4);
    Processor::raw_io_out(PCI_DATA as u64, data as u64, 4);
}
pub fn pci_read_word(bus: u8, slot: u8, func: u8, offset: u8) -> u16 {
    ((pci_read_dword(bus, slot, func, offset) >> ((offset & 2) * 8)) & 0xFFFF) as u16
}
pub fn pci_write_word(bus: u8, slot: u8, func: u8, offset: u8, data: u16) {
    let mut val = pci_read_dword(bus, slot, func, offset);
    val &= !(0xFFFFu32 << ((offset & 2) * 8));
    val |= (data as u32) << ((offset & 2) * 8);
    pci_write_dword(bus, slot, func, offset, val);
}
pub fn pci_read_byte(bus: u8, slot: u8, func: u8, offset: u8) -> u8 {
    ((pci_read_dword(bus, slot, func, offset) >> ((offset & 3) * 8)) & 0xFF) as u8
}
pub fn pci_write_byte(bus: u8, slot: u8, func: u8, offset: u8, data: u8) {
    let mut val = pci_read_dword(bus, slot, func, offset);
    val &= !(0xFFu32 << ((offset & 3) * 8));
    val |= (data as u32) << ((offset & 3) * 8);
    pci_write_dword(bus, slot, func, offset, val);
}

#[derive(Debug)]
struct PCI_INFO {
    root_bus: u64,
    seg: u64
}

static PCI_INF: Once<PCI_INFO> = Once::new();
pub fn pci_init() {
    unsafe {
        let mut table: uacpi_table = core::mem::zeroed();
        let status = uacpi_table_find_by_signature(ACPI_MCFG_SIGNATURE.as_ptr().cast(), &mut table);
        if status != UACPI_STATUS_OK {
            println!("pci: no MCFG table, status={}", status);
            return;
        }

        let mcfg = table.__bindgen_anon_1.virt_addr as *const acpi_mcfg;
        let num_entries = ((*mcfg).hdr.length as usize - 44) / 16;
        if num_entries == 0 {
            println!("pci: MCFG has no entries");
            uacpi_table_unref(&mut table); // clanker told me to use
            return;
        }

        let first = &*(*mcfg).entries.as_ptr();
        PCI_INF.call_once(|| PCI_INFO { root_bus: first.start_bus as u64, seg: first.segment as u64 });

        uacpi_table_unref(&mut table);
        println!("pci: {:?}", PCI_INF.get().unwrap());
    }
}