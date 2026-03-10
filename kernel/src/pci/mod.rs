use core::fmt::Debug;

use alloc::vec::Vec;
use nyaux_uacpi_bindings::{ACPI_MCFG_SIGNATURE, UACPI_STATUS_OK, acpi_mcfg, uacpi_table, uacpi_table_find_by_signature, uacpi_table_unref};

use crate::{arch::{Arch, Processor}, early_init_pagemap, memory::vmm::VMMFlags, println, util::Once};

pub fn pci_read_dword(bus: u8, slot: u8, func: u8, offset: u16) -> u32 {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u32>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    return unsafe {add.read_volatile()};
}
pub fn pci_write_dword(bus: u8, slot: u8, func: u8, offset: u16, data: u32) {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u32>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    unsafe {add.write_volatile(data)};
}
pub fn pci_read_word(bus: u8, slot: u8, func: u8, offset: u16) -> u16 {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u16>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    return unsafe {add.read_volatile()};
}
pub fn pci_write_word(bus: u8, slot: u8, func: u8, offset: u16, data: u16) {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u16>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    unsafe {add.write_volatile(data)};
}
pub fn pci_read_byte(bus: u8, slot: u8, func: u8, offset: u16) -> u8 {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u8>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    return unsafe {add.read_volatile()};
}
pub fn pci_write_byte(bus: u8, slot: u8, func: u8, offset: u16, data: u8) {
    let info = PCI_INF.get().unwrap();
    let add = unsafe {
    core::ptr::with_exposed_provenance_mut::<u8>(info.ecam_base_addr as usize).byte_add(((bus as usize - info.root_bus as usize) << 20) + ((slot as usize) << 15) + ((func as usize) << 12) + offset as usize)};
    unsafe {add.write_volatile(data)};
}

#[derive(Debug)]
struct PCI_INFO {
    root_bus: u64,
    seg: u64,
    ecam_base_addr: u64 // ensure you cast back to a ptr with expose_providence()
}


// vendor_name and class_name and prog_if_name are clanker made but they seem right
// plus dont expect me to manually write it myself
fn vendor_name(id: u16) -> &'static str {
    match id {
        0x8086 => "Intel",
        0x1234 => "QEMU",
        0x1022 => "AMD",
        0x10DE => "NVIDIA",
        0x1AF4 => "VirtIO",
        0x1B36 => "QEMU PCIE",
        _ => "Unknown",
    }
}

fn class_name(class: u8, subclass: u8) -> &'static str {
    match (class, subclass) {
        (0x00, 0x00) => "Non-VGA-Compatible Unclassified Device",
        (0x00, 0x01) => "VGA-Compatible Unclassified Device",
        (0x01, 0x00) => "SCSI Bus Controller",
        (0x01, 0x01) => "IDE Controller",
        (0x01, 0x02) => "Floppy Disk Controller",
        (0x01, 0x03) => "IPI Bus Controller",
        (0x01, 0x04) => "RAID Controller",
        (0x01, 0x05) => "ATA Controller",
        (0x01, 0x06) => "Serial ATA Controller",
        (0x01, 0x07) => "Serial Attached SCSI Controller",
        (0x01, 0x08) => "Non-Volatile Memory Controller",
        (0x01, 0x80) => "Other Mass Storage Controller",
        (0x02, 0x00) => "Ethernet Controller",
        (0x02, 0x01) => "Token Ring Controller",
        (0x02, 0x02) => "FDDI Controller",
        (0x02, 0x03) => "ATM Controller",
        (0x02, 0x04) => "ISDN Controller",
        (0x02, 0x05) => "WorldFip Controller",
        (0x02, 0x06) => "PICMG 2.14 Multi Computing Controller",
        (0x02, 0x07) => "Infiniband Controller",
        (0x02, 0x08) => "Fabric Controller",
        (0x02, 0x80) => "Other Network Controller",
        (0x03, 0x00) => "VGA Compatible Controller",
        (0x03, 0x01) => "XGA Controller",
        (0x03, 0x02) => "3D Controller",
        (0x03, 0x80) => "Other Display Controller",
        (0x04, 0x00) => "Multimedia Video Controller",
        (0x04, 0x01) => "Multimedia Audio Controller",
        (0x04, 0x02) => "Computer Telephony Device",
        (0x04, 0x03) => "Audio Device",
        (0x04, 0x80) => "Other Multimedia Controller",
        (0x05, 0x00) => "RAM Controller",
        (0x05, 0x01) => "Flash Controller",
        (0x05, 0x80) => "Other Memory Controller",
        (0x06, 0x00) => "Host Bridge",
        (0x06, 0x01) => "ISA Bridge",
        (0x06, 0x02) => "EISA Bridge",
        (0x06, 0x03) => "MCA Bridge",
        (0x06, 0x04) => "PCI-to-PCI Bridge",
        (0x06, 0x05) => "PCMCIA Bridge",
        (0x06, 0x06) => "NuBus Bridge",
        (0x06, 0x07) => "CardBus Bridge",
        (0x06, 0x08) => "RACEway Bridge",
        (0x06, 0x09) => "PCI-to-PCI Bridge",
        (0x06, 0x0A) => "InfiniBand-to-PCI Host Bridge",
        (0x06, 0x80) => "Other Bridge",
        (0x07, 0x00) => "Serial Controller",
        (0x07, 0x01) => "Parallel Controller",
        (0x07, 0x02) => "Multiport Serial Controller",
        (0x07, 0x03) => "Modem",
        (0x07, 0x04) => "IEEE 488.1/2 (GPIB) Controller",
        (0x07, 0x05) => "Smart Card Controller",
        (0x07, 0x80) => "Other Simple Communication Controller",
        (0x08, 0x00) => "PIC",
        (0x08, 0x01) => "DMA Controller",
        (0x08, 0x02) => "Timer",
        (0x08, 0x03) => "RTC Controller",
        (0x08, 0x04) => "PCI Hot-Plug Controller",
        (0x08, 0x05) => "SD Host Controller",
        (0x08, 0x06) => "IOMMU",
        (0x08, 0x80) => "Other Base System Peripheral",
        (0x09, 0x00) => "Keyboard Controller",
        (0x09, 0x01) => "Digitizer Pen",
        (0x09, 0x02) => "Mouse Controller",
        (0x09, 0x03) => "Scanner Controller",
        (0x09, 0x04) => "Gameport Controller",
        (0x09, 0x80) => "Other Input Device Controller",
        (0x0A, 0x00) => "Generic Docking Station",
        (0x0A, 0x80) => "Other Docking Station",
        (0x0B, 0x00) => "386 Processor",
        (0x0B, 0x01) => "486 Processor",
        (0x0B, 0x02) => "Pentium Processor",
        (0x0B, 0x03) => "Pentium Pro Processor",
        (0x0B, 0x10) => "Alpha Processor",
        (0x0B, 0x20) => "PowerPC Processor",
        (0x0B, 0x30) => "MIPS Processor",
        (0x0B, 0x40) => "Co-Processor",
        (0x0B, 0x80) => "Other Processor",
        (0x0C, 0x00) => "FireWire (IEEE 1394) Controller",
        (0x0C, 0x01) => "ACCESS Bus Controller",
        (0x0C, 0x02) => "SSA",
        (0x0C, 0x03) => "USB Controller",
        (0x0C, 0x04) => "Fibre Channel",
        (0x0C, 0x05) => "SMBus Controller",
        (0x0C, 0x06) => "InfiniBand Controller",
        (0x0C, 0x07) => "IPMI Interface",
        (0x0C, 0x08) => "SERCOS Interface",
        (0x0C, 0x09) => "CANbus Controller",
        (0x0C, 0x80) => "Other Serial Bus Controller",
        (0x0D, 0x00) => "iRDA Compatible Controller",
        (0x0D, 0x01) => "Consumer IR Controller",
        (0x0D, 0x10) => "RF Controller",
        (0x0D, 0x11) => "Bluetooth Controller",
        (0x0D, 0x12) => "Broadband Controller",
        (0x0D, 0x20) => "Ethernet Controller (802.1a)",
        (0x0D, 0x21) => "Ethernet Controller (802.1b)",
        (0x0D, 0x80) => "Other Wireless Controller",
        (0x0E, 0x00) => "I20 Intelligent Controller",
        (0x0F, 0x01) => "Satellite TV Controller",
        (0x0F, 0x02) => "Satellite Audio Controller",
        (0x0F, 0x03) => "Satellite Voice Controller",
        (0x0F, 0x04) => "Satellite Data Controller",
        (0x10, 0x00) => "Network Encryption/Decryption",
        (0x10, 0x10) => "Entertainment Encryption/Decryption",
        (0x10, 0x80) => "Other Encryption Controller",
        (0x11, 0x00) => "DPIO Modules",
        (0x11, 0x01) => "Performance Counters",
        (0x11, 0x10) => "Communication Synchronizer",
        (0x11, 0x20) => "Signal Processing Management",
        (0x11, 0x80) => "Other Signal Processing Controller",
        _ => "Unknown",
    }
}

fn prog_if_name(class: u8, subclass: u8, prog_if: u8) -> &'static str {
    match (class, subclass, prog_if) {
        (0x01, 0x01, 0x00) => "ISA Compatibility mode-only",
        (0x01, 0x01, 0x05) => "PCI native mode-only",
        (0x01, 0x01, 0x0A) => "ISA Compat, supports PCI native switch",
        (0x01, 0x01, 0x0F) => "PCI native, supports ISA compat switch",
        (0x01, 0x01, 0x80) => "ISA Compatibility mode-only, bus mastering",
        (0x01, 0x01, 0x85) => "PCI native mode-only, bus mastering",
        (0x01, 0x01, 0x8A) => "ISA Compat, PCI native switch, bus mastering",
        (0x01, 0x01, 0x8F) => "PCI native, ISA compat switch, bus mastering",
        (0x01, 0x05, 0x20) => "Single DMA",
        (0x01, 0x05, 0x30) => "Chained DMA",
        (0x01, 0x06, 0x00) => "Vendor Specific Interface",
        (0x01, 0x06, 0x01) => "AHCI 1.0",
        (0x01, 0x06, 0x02) => "Serial Storage Bus",
        (0x01, 0x07, 0x00) => "SAS",
        (0x01, 0x07, 0x01) => "Serial Storage Bus",
        (0x01, 0x08, 0x01) => "NVMHCI",
        (0x01, 0x08, 0x02) => "NVM Express",
        (0x03, 0x00, 0x00) => "VGA Controller",
        (0x03, 0x00, 0x01) => "8514-Compatible Controller",
        (0x06, 0x04, 0x00) => "Normal Decode",
        (0x06, 0x04, 0x01) => "Subtractive Decode",
        (0x06, 0x08, 0x00) => "Transparent Mode",
        (0x06, 0x08, 0x01) => "Endpoint Mode",
        (0x06, 0x09, 0x40) => "Semi-Transparent, Primary towards host CPU",
        (0x06, 0x09, 0x80) => "Semi-Transparent, Secondary towards host CPU",
        (0x07, 0x00, 0x00) => "8250-Compatible",
        (0x07, 0x00, 0x01) => "16450-Compatible",
        (0x07, 0x00, 0x02) => "16550-Compatible",
        (0x07, 0x00, 0x03) => "16650-Compatible",
        (0x07, 0x00, 0x04) => "16750-Compatible",
        (0x07, 0x00, 0x05) => "16850-Compatible",
        (0x07, 0x00, 0x06) => "16950-Compatible",
        (0x07, 0x01, 0x00) => "Standard Parallel Port",
        (0x07, 0x01, 0x01) => "Bi-Directional Parallel Port",
        (0x07, 0x01, 0x02) => "ECP 1.X Compliant Parallel Port",
        (0x07, 0x01, 0x03) => "IEEE 1284 Controller",
        (0x07, 0x01, 0xFE) => "IEEE 1284 Target Device",
        (0x07, 0x03, 0x00) => "Generic Modem",
        (0x07, 0x03, 0x01) => "Hayes 16450-Compatible",
        (0x07, 0x03, 0x02) => "Hayes 16550-Compatible",
        (0x07, 0x03, 0x03) => "Hayes 16650-Compatible",
        (0x07, 0x03, 0x04) => "Hayes 16750-Compatible",
        (0x08, 0x00, 0x00) => "Generic 8259-Compatible",
        (0x08, 0x00, 0x01) => "ISA-Compatible",
        (0x08, 0x00, 0x02) => "EISA-Compatible",
        (0x08, 0x00, 0x10) => "I/O APIC",
        (0x08, 0x00, 0x20) => "I/O(x) APIC",
        (0x08, 0x01, 0x00) => "Generic 8237-Compatible",
        (0x08, 0x01, 0x01) => "ISA-Compatible",
        (0x08, 0x01, 0x02) => "EISA-Compatible",
        (0x08, 0x02, 0x00) => "Generic 8254-Compatible",
        (0x08, 0x02, 0x01) => "ISA-Compatible",
        (0x08, 0x02, 0x02) => "EISA-Compatible",
        (0x08, 0x02, 0x03) => "HPET",
        (0x08, 0x03, 0x00) => "Generic RTC",
        (0x08, 0x03, 0x01) => "ISA-Compatible",
        (0x09, 0x04, 0x00) => "Generic",
        (0x09, 0x04, 0x10) => "Extended",
        (0x0C, 0x00, 0x00) => "Generic",
        (0x0C, 0x00, 0x10) => "OHCI",
        (0x0C, 0x03, 0x00) => "UHCI",
        (0x0C, 0x03, 0x10) => "OHCI",
        (0x0C, 0x03, 0x20) => "EHCI (USB2)",
        (0x0C, 0x03, 0x30) => "xHCI (USB3)",
        (0x0C, 0x03, 0x80) => "Unspecified",
        (0x0C, 0x03, 0xFE) => "USB Device (not host controller)",
        (0x0C, 0x07, 0x00) => "SMIC",
        (0x0C, 0x07, 0x01) => "Keyboard Controller Style",
        (0x0C, 0x07, 0x02) => "Block Transfer",
        _ => "Unknown",
    }
}

static PCI_INF: Once<PCI_INFO> = Once::new();
fn pci_device_print(bus: u8, slot: u8, func: u8, ve: u16) {
    let be = pci_read_dword(bus, slot, func, 0x8) as usize;
    let prog_if = be >> 8;
    let subcl = be >> 16;
    let class = be >> 24;
    println!("PCI {} via {}", class_name(class as u8, subcl as u8), prog_if_name(class as u8, subcl as u8, prog_if as u8))
    
}
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
        let ecam_region_size = ((first.end_bus as usize - first.start_bus as usize) + 1) << 20; // each bus is 1mb or wtv
        let mut s = Vec::new();
        for i in (first.address..(first.address + (ecam_region_size as u64))).step_by(Processor::PAGE_SIZE) {
            s.push(i);
        }
        let virt = early_init_pagemap!().vmm_alloc_with_phys(ecam_region_size as usize, VMMFlags::WRITE | VMMFlags::NOCACHE,s ).unwrap();
        PCI_INF.call_once(|| PCI_INFO { root_bus: first.start_bus as u64, seg: first.segment as u64, ecam_base_addr: virt.expose_provenance() as u64});
        let sb = first.start_bus;
        uacpi_table_unref(&mut table);
        for i in 0..32 {
            let ve = pci_read_word(sb, i, 0, 0x0);
            if ve == 0xFFFF {
                continue;
            }
            let he_type = (pci_read_dword(sb, i, 0, 0xC) >> 16) & 0xFF;
            if he_type & (1 << 7) != 0 {
                for j in 0..8 {
                    let v = pci_read_word(sb, i, j, 0x0);
                    if v == 0xFFFF {
                        continue;
                    }
                    pci_device_print(sb, i, j, ve);
                }
            } else {
                pci_device_print(sb, i, 0, ve);
            }

        }
    }
    
}