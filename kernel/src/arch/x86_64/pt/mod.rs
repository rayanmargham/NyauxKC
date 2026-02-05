use bitflags::bitflags;
use bytemuck::{Pod, Zeroable};
use limine::paging::{self, Mode};

use crate::{
    HHDM_REQUEST, arch::PAGING_MODE_REQUEST, memory::pmm::allocate_page, print, println, status,
};
bitflags! {

    #[derive(PartialEq, Debug)]
    struct PT: u8 {
        const PRESENT   = 1 << 0;
        const WRITE     = 1 << 1;
        const USER      = 1 << 2;
        const NEXEC     = 1 << 3;
        const GLOBAL    = 1 << 4;
        const GIGAPAGE  = 1 << 5;
        const MEGAPAGE  = 1 << 6;
    }
}
fn get_cr3() -> u64 {
    let bro: u64;
    unsafe {
        core::arch::asm!("mov {}, cr3" ,out(reg) bro);
    }
    bro
}
#[repr(C)]
#[derive(core::fmt::Debug, Copy, Clone)]
pub struct PTENT(*mut u64);
impl PT {
    fn new(table: &PTENT, lvl: u8) -> PT {
        let check = table.0.addr();
        let mut r: PT = PT::empty();
        if (check & 1) != 0 {
            r |= PT::PRESENT;
        }
        println!("{:?}", check & (1 << 63));
        if (check & (1 << 1)) != 0 {
            r |= PT::WRITE
        }
        if (check & (1 << 2)) != 0 {
            println!("che: {:b}", check);
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
    fn build_table(phyaddress: u64, permissions: PT, lvl: u8) -> PTENT {
        let arranged_bro: u64 = permissions.build_permissions(lvl) | {
            match lvl {
                4 => phyaddress << 12,
                3 => {
                    if permissions.contains(PT::GIGAPAGE) {
                        phyaddress << 30
                    } else {
                        phyaddress << 12
                    }
                }
                2 => {
                    if permissions.contains(PT::MEGAPAGE) {
                        phyaddress << 21
                    } else {
                        phyaddress << 12
                    }
                }
                1 => phyaddress << 12,
                _ => 0,
            }
        };
        PTENT(core::ptr::without_provenance_mut::<u64>(
            arranged_bro as usize,
        ))
    }
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
        for j in (0..=4).rev() {
            if j == 0 {
                return Ok(PTENT(unsafe {
                    targetted_table
                        .as_mut_ptr()
                        .cast::<u8>()
                        .sub(HHDM_REQUEST.get_response().unwrap().offset() as usize)
                        .cast::<u64>()
                        .map_addr(|a| (a & !0xFFF) & !(1 << 63))
                }));
            }
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
            println!("h");
            let mut next = targetted_table[vi as usize];
            println!("d");
            if next.0.is_null() {
                if alloc {
                    let page = allocate_page();
                    println!("bef");
                    targetted_table[vi as usize] = PTENT(
                        unsafe {
                            page.cast::<u8>()
                                .sub(HHDM_REQUEST.get_response().unwrap().offset() as usize)
                        }
                        .map_addr(|a| a | (PT::PRESENT | PT::USER | PT::WRITE).bits() as usize)
                        .cast::<u64>(),
                    );
                    println!("a");
                    next = targetted_table[vi as usize];
                } else {
                    return Err("bru");
                }
            }
            let perm = PT::new(&next, j);

            println!("perm: {:?}", perm);
            if perm.contains(PT::MEGAPAGE) || perm.contains(PT::GIGAPAGE) || j == 1 {
                return Ok(PTENT(core::ptr::without_provenance_mut::<u64>(
                    (next.0 as usize & !0xFFF) & !(1 << 63),
                )));
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
        let h = bro.unwrap().0;
        unsafe {
            h.write(0);
            core::arch::asm!("invlpg {}", in(reg) h);
        }
    }
}

pub fn pt_init() {
    println!("trying shit out");
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
        (get_cr3() as usize & !0xFFF) & !(1 << 63),
    ));

    println!(
        "got physical address {:p}",
        limine_pml4.get_table(0xffffffff80015000, false).unwrap().0
    );
    let test = PTENT(unsafe {
        allocate_page()
            .cast::<u8>()
            .sub(HHDM_REQUEST.get_response().unwrap().offset() as usize)
            .cast::<u64>()
    });
    println!("HERE: 0x{:x}", unsafe {
        test.get_table(0xABCD, true)
            .unwrap()
            .0
            .byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize)
            .addr()
    });
    unsafe {
        test.get_table(0xABCD, false)
            .unwrap()
            .0
            .byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize)
            .write(0xBA116767)
    };
    unsafe {
        status!(
            "page table mapping function works? 0x{:x}",
            test.get_table(0xABCD, false)
                .unwrap()
                .0
                .byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize)
                .read()
        )
    }
}
