use core::ptr::addr_of;

use bitflags::bitflags;

use crate::{HHDM_REQUEST, KERNELADDR_REQUEST, KS, align_up, arch::{Arch, PAGING_MODE_REQUEST, Processor}, memory::{pmm::{MEMMAP_REQUEST, allocate_page, deallocate_page}, vmm::{Pagemap, VMMFlags}}, println};
use alloc::vec::Vec;

pub fn phys_to_virt<T>(phys: u64) -> *mut T {
    unsafe {
        core::ptr::with_exposed_provenance_mut::<T>(phys as usize)
            .byte_add(HHDM_REQUEST.response().unwrap().offset as usize)
    }
}
const fn extract_phys_from_pte(pte: u64) -> u64 {
    ((pte >> 10) & 0xFFF_FFFF_FFFF) << 12
}
static mut SV_LEVELS: u8 = 3;
bitflags! {
    pub struct PT: u8 {
        const VALID = 1 << 0;
        const READ = 1 << 1;
        const WRITE = 1 << 2;
        const EXECUTE = 1 << 3;
        const USER = 1 << 4;
        const GLOBAL = 1 << 5;
        
    }
}
#[repr(transparent)]
#[derive(Debug, Clone, Copy)]
pub struct PTENT(pub u64);
impl PT {
    pub fn from_vmmflags(fla: VMMFlags) -> PT {
        // vmm flags dont have read flag yet
        let mut val: PT = PT::VALID | PT::READ;
        if fla.contains(VMMFlags::WRITE) {
            val |= PT::WRITE;
        }
        if fla.contains(VMMFlags::EXECUTABLE) {
            val |= PT::EXECUTE;
        }
        if fla.contains(VMMFlags::GLOBAL) {
            val |= PT::GLOBAL;
        }
        if fla.contains(VMMFlags::USER) {
            val |= PT::USER;
        }
        val
    }
    fn new(table: &PTENT) -> PT {
        let see = table.0;
        let mut p: PT = PT::empty();
        if (see & 1) != 0 {
            p |= PT::VALID;
        }
        if (see & (1 << 1)) != 0 {
            p |= PT::READ;
        }
        if (see & (1 << 2)) != 0 {
            p |= PT::WRITE;
        }
        if (see & (1 << 3)) != 0 {
            p |= PT::EXECUTE;
        }
        if (see & (1 << 4)) != 0 {
            p |= PT::USER;
        }
        if (see & (1 << 5)) != 0 {
            p |= PT::GLOBAL;
        }
        p
    }
    const fn build_permissions(&self) -> u64 {
        let mut final_val: u64 = 0;
        if self.contains(PT::VALID) {
            final_val |= 1;
        }
        if self.contains(PT::READ) {
            final_val |= 1 << 1;
        }
        if self.contains(PT::WRITE) {
            final_val |= 1 << 2;
        }
        if self.contains(PT::EXECUTE) {
            final_val |= 1 << 3;
        }
        if self.contains(PT::USER) {
            final_val |= 1 << 4;
        }
        if self.contains(PT::GLOBAL) {
            final_val |= 1 << 5;
        }
        final_val
    }
    
}
impl PTENT {
    // @brief returns with hhdm offset added
    pub fn get_table(&self, virt: u64, levels: u8, target_level: u8, alloc: bool) -> Result<*mut u64, &'static str>{
        let mut table: *mut u64 = phys_to_virt(self.0);
        for level in (target_level..levels).rev() {
            let shift = 12 + (level as usize * 9); // claude told me this is better to do instead of hardcoding
            let index = ((virt >> shift) & 0x1FF) as usize;
            let ble = unsafe {
                table.add(index)
            };
            let data = unsafe {
                ble.read()
            };
            let perm = PT::new(&PTENT(data));
            if level == target_level || perm.intersects(PT::READ | PT::WRITE | PT::EXECUTE) && perm.contains(PT::VALID){
                return Ok(ble);
            }
            if !perm.contains(PT::VALID) {
                if alloc {
                    let page = unsafe {allocate_page().byte_sub(HHDM_REQUEST.response().unwrap().offset as usize).expose_provenance()};
                    let ppn = page >> 12;
                    unsafe {
                        ble.write(
                            ((ppn << 10) | 1) as u64
                        );
                    }
                } else {
                    return Err("gg broski")
                }
                let data = unsafe {
                    ble.read()
                };
                let lets_see = extract_phys_from_pte(data);
                table = phys_to_virt(lets_see);
            } else {
                let lets_see = extract_phys_from_pte(data);
                table = phys_to_virt(lets_see);
            }
        }
        Err("failed")
    }

    pub fn map4kib(&self, virt: u64, phys: u64, flags: PT) -> Result<(), &'static str> {
        let yum = self.get_table(virt, unsafe {SV_LEVELS}, 0, true)?;
        let ppn = phys >> 12;
        unsafe {
            yum.write((ppn << 10) | flags.build_permissions());
        };
        Ok(())
    } 
    pub fn unmap4kib(&self, virt: u64) {
        let bro = self.get_table(virt, unsafe {SV_LEVELS}, 0, true).unwrap();
        if !bro.is_null() {
            unsafe {
                let page = core::ptr::with_exposed_provenance_mut::<u64>(extract_phys_from_pte(bro.read()) as usize)
                .byte_add(HHDM_REQUEST.response().unwrap().offset as usize);
                deallocate_page(page.cast());
                bro.write(0);
                core::arch::asm!("sfence.vma {}, zero", in(reg) virt);
            }
        }
    }
}
impl Pagemap {
    pub fn archpt(&self) -> PTENT {
        PTENT(self.arch_page.expose_provenance() as u64)
    }
    pub fn arch_map_region_alloc(&self, base: usize, length: usize, flags: VMMFlags) {
        let yo = self.archpt();
        println!("yo is 0x{:x}", yo.0);
        for i in (base..(base + length)).step_by(Processor::PAGE_SIZE) {
            yo.map4kib(
                i as u64, 
                allocate_page().addr() as u64 - HHDM_REQUEST.response().unwrap().offset, PT::from_vmmflags(flags)).unwrap();
        }
    }
    pub fn arch_unmap_region(&self, base: usize, length: usize) {
        let yo = self.archpt();
        for i in (base..(base + length)).step_by(Processor::PAGE_SIZE) {
            yo.unmap4kib(i as u64);
        }
    }

    pub fn arch_map_region(&self, base: usize, length: usize, phys: Vec<u64>, flags: crate::memory::vmm::VMMFlags) {
        let yo = self.archpt();
        for (idx, i) in (base..(base + length)).step_by(Processor::PAGE_SIZE).enumerate() {
            let pa = phys[idx];
            yo.map4kib(
                i as u64, pa, PT::from_vmmflags(flags)).unwrap();
        }
    }
}
pub fn pt_init() -> (usize, usize) {
    let mode = PAGING_MODE_REQUEST.response().unwrap().mode;
    match mode {
        limine_boot::paging::PagingMode::RISCV_SV39 => {
            println!("we will use sv39");
            unsafe {SV_LEVELS = 3};
        },
        limine_boot::paging::PagingMode::RISCV_SV48=> {
            println!("we are going to use sv48");
            unsafe {
                SV_LEVELS = 4;
            }
        },
        limine_boot::paging::PagingMode::RISCV_SV57 => {
            println!("we are going to use sv57");
            unsafe {
                SV_LEVELS = 5;
            }
        }
        _ => {
            
        }
    }
    let yo = PTENT(allocate_page().expose_provenance() as u64 - HHDM_REQUEST.response().unwrap().offset);
    let kaddrv = KERNELADDR_REQUEST.response().unwrap().virtual_base as usize;
    let kaddrp = KERNELADDR_REQUEST.response().unwrap().physical_base as usize;
    let kernel_size = align_up(addr_of!(KS) as u64 - kaddrv as u64, Processor::PAGE_SIZE as u64) as usize;
    println!("kernel size 0x{:x}", kernel_size);
    println!("mapping kernel");
    for i in (0..kernel_size).step_by(Processor::PAGE_SIZE) {
        //println!("virtual address to map of kernel 0x{:x}", i);
        yo.map4kib(
            (kaddrv + i) as u64,
            (kaddrp + i) as u64,
            PT::GLOBAL | PT::VALID | PT::WRITE | PT::READ | PT::EXECUTE,
        )
        .unwrap();
    }
    let mut max_hhdm_phys = 0 as usize;
     if let Some(memap_res) =MEMMAP_REQUEST.response() {
        for i in memap_res.entries() {
            match i.type_ {
                limine_boot::memmap::MEMMAP_MAPPED_RESERVED
                | limine_boot::memmap::MEMMAP_USABLE
                | limine_boot::memmap::MEMMAP_BOOTLOADER_RECLAIMABLE
                | limine_boot::memmap::MEMMAP_FRAMEBUFFER
                | limine_boot::memmap::MEMMAP_EXECUTABLE_AND_MODULES
                | limine_boot::memmap::MEMMAP_ACPI_NVS
                | limine_boot::memmap::MEMMAP_ACPI_RECLAIMABLE => {
                    max_hhdm_phys = (i.base + i.length).max(max_hhdm_phys as u64) as usize;
                },
                _ => {}
            }
        }
    }
        max_hhdm_phys = align_up(max_hhdm_phys as u64, Processor::PAGE_SIZE as u64) as usize;
        for i in (0..max_hhdm_phys).step_by(Processor::PAGE_SIZE) {
        let flags = {
                PT::GLOBAL | PT::VALID | PT::WRITE | PT::READ | PT::EXECUTE
        };
        yo.map4kib(
            HHDM_REQUEST.response().unwrap().offset + (i as u64),
            i as u64,
            flags,
        )
        .unwrap();
    }
    let mode: usize= unsafe {
        match SV_LEVELS {
            4 => {
                9
            },
            3 => {
                8
            },
            5 => {
                10
            },
            _ => {
                unreachable!();
            }
        }
    };
    let ppn = yo.0 >> 12;
    let arranged = ppn as usize | ((mode & 0xF) << 60);
    unsafe {
        core::arch::asm!("
        sfence.vma zero, zero");
        core::arch::asm!("
        csrw satp, {}", in(reg) arranged);
        core::arch::asm!("
        sfence.vma zero, zero");
    }
    (max_hhdm_phys, kernel_size)
}