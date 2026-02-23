use core::fmt::Debug;
use core::slice::Iter;

use bytemuck::{Pod, Zeroable};
use limine_boot::request::MemmapRequest;

use crate::arch::{Arch, Processor};
use crate::memory::slab::init_slab;
use crate::{HHDM_REQUEST, print, println};
#[used]
#[unsafe(link_section = ".requests")]
pub static MEMMAP_REQUEST: MemmapRequest=MemmapRequest::new();

#[repr(C)]
#[derive(Zeroable)]
pub struct PMMNode {
    pub next: *mut PMMNode,
}
// CONTAINS PHYSICAL ADDRESSES
static mut FREELIST: Option<*mut PMMNode> = None;

pub fn init() {
    if let Some(memap_response) = MEMMAP_REQUEST.response() {
        for i in memap_response.entries() {
            'a: {
                match i.type_{
                    limine_boot::memmap::MEMMAP_USABLE => {
                        unsafe {
                           
                            for i in (i.base..(i.base + i.length)).step_by(Processor::PAGE_SIZE) {
                                if i == 0 {
                                    continue;
                                }

                                let prev = FREELIST.unwrap_or(core::ptr::null_mut());
                                (i as *mut PMMNode).byte_add(HHDM_REQUEST.response().unwrap().offset as usize).write(PMMNode { next: prev });
                                FREELIST = Some(i as *mut PMMNode);
                            }
                        };
                    }
                    _ => {}
                }
            }
        }
    };
    println!(
        "i suppose it worked?? freelist created? {:?}",
        FREELIST.unwrap()
    );
    init_slab();
}

pub fn allocate_page() -> *mut () {
    unsafe {
        if let Some(phy) = FREELIST {
            let bro = phy.byte_add(HHDM_REQUEST.response().unwrap().offset as usize);
            FREELIST = Some((*bro).next);
            
            bro.cast::<u8>().write_bytes(0, Processor::PAGE_SIZE);
            return bro as *mut ();
        } else {
            panic!("gbr");
        }
    };
}

pub fn deallocate_page(ptr: *mut ()) {
    unsafe {
        println!("ptr addr: 0x{:x}", ptr.addr());
        let o = FREELIST.unwrap();
        ptr.cast::<u8>().write_bytes(0, Processor::PAGE_SIZE);
        let real = ptr as *mut PMMNode;
        real.write(PMMNode { next: o });
        FREELIST = Some(real.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize));
    }
}
