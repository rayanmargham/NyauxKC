use core::ffi::{CStr, c_void};

use nyaux_uacpi_bindings::{UACPI_PREDEFINED_NAMESPACE_SB, uacpi_find_devices_at, uacpi_iteration_callback, uacpi_iteration_decision, uacpi_namespace_get_predefined, uacpi_namespace_node, uacpi_u32};

use crate::arch::{Arch, Processor};

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
unsafe extern "C" fn pci_itcb(
        user: *mut ::core::ffi::c_void,
        node: *mut uacpi_namespace_node,
        node_depth: uacpi_u32,
    ) -> uacpi_iteration_decision {
            
    }
pub fn pci_init() {
    unsafe {
        let root_ids:  = [
            c"PNP0A03".as_ptr(),
            c"PNP0A08".as_ptr(),
            core::ptr::null()
        ];
        let num = 0;
        uacpi_iteration_callback
        unsafe {
        uacpi_find_devices_at(uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB), root_ids.as_ptr(), cb, (&raw mut num).cast::<c_void>())};
    }
}