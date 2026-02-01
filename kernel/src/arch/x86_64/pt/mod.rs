use bitflags::bitflags;
use bytemuck::Pod;
use limine::paging::{self, Mode};

use crate::{arch::PAGING_MODE_REQUEST, println};
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
#[repr(C)]
pub struct PTENT(u64);
impl PT {
    fn new(table: PTENT, lvl: u8) -> PT {
        let check = table.0;
        let mut r: PT = PT::empty();
        if check & 1 == 1{
            r |= PT::PRESENT;
        }
        println!("{:?}", check & (1 << 63));
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
            },
            2 => {
                if check & (1 << 7) != 0 {
                    r |= PT::MEGAPAGE;
                    if check & (1 << 8) != 0 {
                        r |= PT::GLOBAL;
                    }
                }
            },
            1 => {
                if check & (1 << 8) != 0 {
                    r |= PT::GLOBAL;
                }
            }
            _ => {}
        }
        r
    }
    const fn build_permissions(&self, lvl: u8) -> u64{
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


            },
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
                4 => {
                    phyaddress << 12
                },
                3 => {
                    if permissions.contains(PT::GIGAPAGE) {
                        phyaddress << 30
                    } else {
                        phyaddress << 12
                    }
                },
                2 => {
                    if permissions.contains(PT::MEGAPAGE) {
                        phyaddress << 21
                    } else {
                        phyaddress << 12
                    }
                },
                1 => {
                    phyaddress << 12
                },
                _ => {0}
            }
        };
        PTENT(arranged_bro)
    }
    // fn rallocate_table(pml4: PTENT, virt: u64, phys: u64, permissions: PT) {
    //     pml4.0.
    // }
    

}

pub fn pt_init() {
    println!("trying shit out");
    let pagingresponse = PAGING_MODE_REQUEST.get_response().unwrap();
    match pagingresponse.mode() {
        Mode::FOUR_LEVEL => {
            println!("Booted with 4-lvl paging")
        },
        Mode::FIVE_LEVEL => {
            println!("Booted with 5-lvl paging");
        }
        _ => {
            println!("unknown paging level we booted at");
        }
    }
    println!("{:b}", 0xFFFFF);
    let test: PT = PT::PRESENT | PT::WRITE | PT::GLOBAL | PT::NEXEC;
    let new = PT::new(PTENT(test.build_permissions(4)), 4);
    assert_eq!(test, new);
    println!("built thingy {:b}", PTENT::build_table(0xFFFF, PT::PRESENT | PT::WRITE | PT::GLOBAL | PT::NEXEC, 4).0);


}