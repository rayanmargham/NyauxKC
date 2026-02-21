use core::ptr::addr_of;

use bitflags::bitflags;
use bytemuck::{Pod, Zeroable};
use limine::{memory_map::EntryType, paging::{self, Mode}, request::ExecutableAddressRequest};

use crate::{
    HHDM_REQUEST, KS, align_up, arch::{Arch, PAGING_MODE_REQUEST, Processor}, memory::{pmm::{MEMMAP_REQUEST, allocate_page, deallocate_page}, vmm::{Pagemap, VMMFlags, kermap}}, print, println, status
};
const fn setup_phys_for_pte(phys: u64) -> u64 {
    phys & 0x000F_FFFF_FFFF_F000 // abomination, but nyaux code does it, i must do it too. i misunderstood some things
}
#[used]
#[unsafe(link_section = ".requests")]
pub static KERNELADDR_REQUEST: ExecutableAddressRequest = ExecutableAddressRequest::new();
bitflags! {

    #[derive(PartialEq, Debug)]
    pub struct PT: u8 {
        const PRESENT   = 1 << 0;
        const WRITE     = 1 << 1;
        const USER      = 1 << 2;
        const NEXEC     = 1 << 3;
        const GLOBAL    = 1 << 4;
        const GIGAPAGE  = 1 << 5;
        const MEGAPAGE  = 1 << 6;
    }
}
pub fn read_cr3() -> u64 {
    let bro: u64;
    unsafe {
        core::arch::asm!("mov {}, cr3" ,out(reg) bro);
    }
    bro
}
fn write_cr3(bro: u64) {
    unsafe {
        core::arch::asm!("mov cr3, {}", in(reg) bro);
    }
}
#[repr(C)]
#[derive(core::fmt::Debug, Copy, Clone)]
pub struct PTENT(pub *mut u64);
impl PT {
    pub fn from_vmmflags(fla: VMMFlags) -> PT {
        let mut val: PT = PT::PRESENT;
        if fla.contains(VMMFlags::WRITE) {
            val |= PT::WRITE;
        }
        if !fla.contains(VMMFlags::EXECUTABLE) {
            val |= PT::NEXEC;
        }
        if fla.contains(VMMFlags::GLOBAL) {
            val |= PT::GLOBAL;
        }
        if fla.contains(VMMFlags::USER) {
            val |= PT::USER
        }
        val
    }
    fn new(table: &PTENT, lvl: u8) -> PT {
        let check = table.0.addr();
        let mut r: PT = PT::empty();
        if (check & 1) != 0 {
            r |= PT::PRESENT;
        }
        if (check & (1 << 1)) != 0 {
            r |= PT::WRITE
        }
        if (check & (1 << 2)) != 0 {
            r |= PT::USER;
        }
        if (check & (1 << 63)) != 0 {
            r |= PT::NEXEC;
        }
        match lvl {
            3 => {
                if check & (1 << 7) != 0 {
                    r |= PT::GIGAPAGE;
                    if check & (1 << 8) != 0 {
                        r |= PT::GLOBAL;
                    }
                }
            }
            2 => {
                if check & (1 << 7) != 0 {
                    r |= PT::MEGAPAGE;
                    if check & (1 << 8) != 0 {
                        r |= PT::GLOBAL;
                    }
                }
            }
            1 => {
                if check & (1 << 8) != 0 {
                    r |= PT::GLOBAL;
                }
            }
            _ => {}
        }
        r
    }
    const fn build_permissions(&self, lvl: u8) -> u64 {
        let mut final_val: u64 = 0;
        if self.contains(PT::PRESENT) {
            final_val |= 1;
        }
        if self.contains(PT::USER) {
            final_val |= 1 << 2;
        }
        if self.contains(PT::WRITE) {
            final_val |= 1 << 1;
        }
        if self.contains(PT::NEXEC) {
            final_val |= 1 << 63;
        }
        match lvl {
            3 => {
                let mut giga = false;
                if self.contains(PT::GIGAPAGE) {
                    giga = true;
                    final_val |= 1 << 7;
                }
                if giga && self.contains(PT::GLOBAL) {
                    final_val |= 1 << 8;
                }
            }
            2 => {
                let mut mega = false;
                if self.contains(PT::MEGAPAGE) {
                    mega = true;
                    final_val |= 1 << 7;
                }
                if mega && self.contains(PT::GLOBAL) {
                    final_val |= 1 << 8;
                }
            }
            1 => {
                if self.contains(PT::GLOBAL) {
                    final_val |= 1 << 8;
                }
            }
            _ => {}
        }
        final_val
    }
}

impl PTENT {
    // fn build_table(phyaddress: u64, permissions: PT, lvl: u8) -> PTENT {
    //     let arranged_bro: u64 = permissions.build_permissions(lvl) | {
    //         match lvl {
    //             4 => phyaddress << 12,
    //             3 => {
    //                 if permissions.contains(PT::GIGAPAGE) {
    //                     phyaddress << 30
    //                 } else {
    //                     phyaddress << 12
    //                 }
    //             }
    //             2 => {
    //                 if permissions.contains(PT::MEGAPAGE) {
    //                     phyaddress << 21
    //                 } else {
    //                     phyaddress << 12
    //                 }
    //             }
    //             1 => phyaddress << 12,
    //             _ => 0,
    //         }
    //     };
    //     PTENT(core::ptr::without_provenance_mut::<u64>(
    //         arranged_bro as usize,
    //     ))
    // }
    // // we assume we are pml4
    fn get_table(&self, virt: u64, alloc: bool) -> Result<PTENT, &'static str> {
        // we needa mask off those 12 bits cause those contain flags, i was confused for an hour over this lmao
        let mut targetted_table = unsafe {
            core::ptr::without_provenance_mut::<[PTENT; 512]>(
                (((self.0 as usize) & !0xFFF) & !(1 << 63))
                    + HHDM_REQUEST.get_response().unwrap().offset() as usize,
            )
            .as_mut()
            .unwrap()
        };
        for j in (1..=4).rev() {

            let vi = virt >> {
                match j {
                    4 => 39,
                    3 => 30,
                    2 => 21,
                    1 => 12,
                    0 => 0,
                    _ => {
                        panic!("unexpected page level");
                    }
                }
            } & 0x1ff;

            let mut next = targetted_table[vi as usize];
            let perm = PT::new(&next, j);
            if perm.contains(PT::MEGAPAGE) || perm.contains(PT::GIGAPAGE) || j == 1 {
                let blah = vi * 8; // get the pte's address and prepare shit
                unsafe {
                return Ok(PTENT(targetted_table.as_mut_ptr().byte_add(blah as usize).byte_sub(HHDM_REQUEST.get_response().unwrap().offset() as usize).cast::<u64>()));}
            }
            if next.0.is_null() {
                if alloc {
                    let page = allocate_page();
                    targetted_table[vi as usize] = PTENT(
                        unsafe {
                            page.cast::<u8>()
                                .sub(HHDM_REQUEST.get_response().unwrap().offset() as usize)
                        }
                        .map_addr(|a| a | (PT::PRESENT | PT::USER | PT::WRITE).bits() as usize)
                        .cast::<u64>(),
                    );
                    next = targetted_table[vi as usize];
                } else {
                    return Err("bru");
                }
            }


            targetted_table = unsafe {
                core::ptr::without_provenance_mut::<[PTENT; 512]>(
                    ((next.0 as usize & !0xFFF) & !(1 << 63))
                        + HHDM_REQUEST.get_response().unwrap().offset() as usize,
                )
                .as_mut()
                .unwrap()
            };
        }

        Err("fuck")
    }
    fn unmap(&self, virt: u64) {
        let bro = self.get_table(virt, false);
        if bro.is_err() {
            return;
        }
        let h = unsafe {bro.unwrap().0.byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize)};
        
        if !h.is_null() {
            unsafe {
                let page = core::ptr::with_exposed_provenance::<u64>(setup_phys_for_pte(h.read()) as usize).byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize).cast_mut();
                deallocate_page(page.cast());
                h.write(0);
                core::arch::asm!("invlpg [{}]", in(reg) virt);
            }
        }
    }
    // @brief this function always maps 4 byte pages
    pub fn map4kib(&self, virt: u64, phys: u64, flags: PT) -> Result<(), &'static str> {
        let answer = self.get_table(virt, true);
        if answer.is_ok() {
            unsafe {
                let p_t = answer.unwrap();
                let calculated_pt_val = setup_phys_for_pte(phys) | PT::build_permissions(&flags, 1) as u64;
                
                p_t.0.byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize).write(calculated_pt_val);
                return Ok(());
            }
        } else {
            return Err(answer.err().unwrap());
        }
    }
}
impl Pagemap {
    pub fn archpt(&self) -> PTENT {
        PTENT(self.arch_page)
    }
    pub fn arch_map_region(&self, base: usize, length: usize, flags: crate::memory::vmm::VMMFlags) {

            
            let yo = self.archpt();
            for i in (base..(base + length)).step_by(Processor::PAGE_SIZE) {
                use crate::{arch::x86_64::pt::PT, memory::pmm::allocate_page};

                yo.map4kib(i as u64, allocate_page().addr() as u64 - HHDM_REQUEST.get_response().unwrap().offset(), PT::from_vmmflags(flags)).unwrap();
                
            } 
    }
    pub fn arch_unmap_region(&self, base: usize, length: usize) {
        let pml = self.archpt();
        for i in (base..(base + length)).step_by(Processor::PAGE_SIZE) {
            pml.unmap(i as u64);
        }
    }
}
pub fn pt_init() -> (usize, usize) {
    let kernel_size = align_up(addr_of!(KS) as u64,Processor::PAGE_SIZE as u64) as usize;
    println!("trying shit out, kernel size 0x{:x}", kernel_size);
    let pagingresponse = PAGING_MODE_REQUEST.get_response().unwrap();
    
    match pagingresponse.mode() {
        Mode::FOUR_LEVEL => {
            println!("Booted with 4-lvl paging")
        }
        Mode::FIVE_LEVEL => {
            println!("Booted with 5-lvl paging");
        }
        _ => {
            println!("unknown paging level we booted at");
        }
    }
    let limine_pml4 = PTENT(core::ptr::without_provenance_mut(
        (read_cr3() as usize & !0xFFF) & !(1 << 63),
    ));

    println!(
        "got physical address {:p}",
        limine_pml4.get_table(0xffffffff80015000, false).unwrap().0
    );
    let pml4= PTENT(unsafe {
        allocate_page()
            .cast::<u8>()
            .sub(HHDM_REQUEST.get_response().unwrap().offset() as usize)
            .cast::<u64>()
    });

    let kaddrv = KERNELADDR_REQUEST.get_response().unwrap().virtual_base() as usize;
    let kaddrp = KERNELADDR_REQUEST.get_response().unwrap().physical_base() as usize;
    for i in (0..kernel_size).step_by(Processor::PAGE_SIZE) {
        //println!("virtual address to map of kernel 0x{:x}", i);
        pml4.map4kib((kaddrv + i) as u64, (kaddrp + i) as u64, PT::GLOBAL | PT::PRESENT | PT::WRITE).unwrap();
    }
    let mut max_hhdm_phys = 0 as usize;
    if let Some(memap_res) = MEMMAP_REQUEST.get_response() {
        for i in memap_res.entries() {
            match i.entry_type {
                EntryType::USABLE | EntryType::BOOTLOADER_RECLAIMABLE | EntryType::EXECUTABLE_AND_MODULES | EntryType::FRAMEBUFFER | EntryType::ACPI_NVS | EntryType::ACPI_RECLAIMABLE => {
                    max_hhdm_phys = (i.base + i.length).max(max_hhdm_phys as u64) as usize;
                },
                _ => {

                }
            }
        }
    }
    max_hhdm_phys = align_up(max_hhdm_phys as u64, Processor::PAGE_SIZE as u64) as usize;
    for i in (0..max_hhdm_phys).step_by(Processor::PAGE_SIZE) {
        pml4.map4kib(HHDM_REQUEST.get_response().unwrap().offset() + (i as u64), i as u64, PT::GLOBAL | PT::PRESENT | PT::WRITE).unwrap();
    }
    println!("writing pml4 address to cr3 {}", pml4.0.addr());
    write_cr3(pml4.0.addr() as u64);
    status!("page tables switched to Nyaux!");
    (max_hhdm_phys, kernel_size)

}
