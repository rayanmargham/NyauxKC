use bitflags::bitflags;
use bytemuck::Pod;
use limine::paging::{self, Mode};

use crate::{arch::PAGING_MODE_REQUEST, println};

bitflags! {
    struct PT: u8 {
        const PRESENT = 1;
        const WRITE = 2;
        const USER = 3;
        const NEXEC = 4;
        const GLOBAL = 5;
        const GIGAPAGE = 6;
        const MEGAPAGE = 7;
    }
}
#[repr(C)]
pub struct PTENT(u64);
impl PT {
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


}