use bitflags::bitflags;

use crate::{arch::PAGING_MODE_REQUEST, memory::vmm::{Pagemap, VMMFlags}, println};




bitflags! {
    struct PT: u8 {
        const VALID = 1 << 0;
        const READ = 1 << 1;
        const WRITE = 1 << 2;
        const EXECUTE = 1 << 3;
        const USER = 1 << 4;
        const GLOBAL = 1 << 5;
        
    }
}
impl Pagemap {
    pub fn arch_map_region(&self, base: usize, length: usize, flags: VMMFlags) {
        todo!()
    }
    pub fn arch_unmap_region(&self, base: usize, length: usize) {
        todo!()
    }
}
pub fn pt_init() -> (usize, usize) {
    let mode = PAGING_MODE_REQUEST.response().unwrap().mode;
    let mut svwhat = false; // sv39 if false, sv48 if true
    match mode {
        limine_boot::paging::PagingMode::RISCV_SV39 => {
            println!("we will use sv39");
        },
        limine_boot::paging::PagingMode::RISCV_SV48 | limine_boot::paging::PagingMode::RISCV_SV57 => {
            println!("we are going to use sv48");
            svwhat = true;
        },
        
        _ => {
            
        }
    }
    todo!()
}