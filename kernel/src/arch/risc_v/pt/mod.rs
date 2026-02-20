use bitflags::bitflags;
use limine::{paging::Mode, request::ExecutableAddressRequest};

use crate::{arch::PAGING_MODE_REQUEST, println};



#[used]
#[unsafe(link_section = ".requests")]
static KERNELADDR_REQUEST: ExecutableAddressRequest = ExecutableAddressRequest::new();
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
pub fn pt_init() {
    let mode = PAGING_MODE_REQUEST.get_response().unwrap().mode();
    let mut svwhat = false; // sv39 if false, sv48 if true
    match mode {
        Mode::SV39 => {
            println!("we will use sv39");
        },
        Mode::SV48 | Mode::SV57 => {
            println!("we are going to use sv48");
            svwhat = true;
        },
        
        _ => {
            
        }
    }
}