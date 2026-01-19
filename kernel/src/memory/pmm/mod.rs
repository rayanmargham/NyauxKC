use core::fmt::Debug;
use core::slice::Iter;

use bytemuck::{Pod, Zeroable};
use limine::memory_map::EntryType;
use limine::request::MemoryMapRequest;

use crate::arch::{Arch, Processor};
use crate::memory::slab::init_slab;
use crate::{HHDM_REQUEST, print, println};
#[used]
#[unsafe(link_section = ".requests")]
static MEMMAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[repr(C)]
#[derive(Zeroable)]
pub struct PMMNode {
    pub next: *mut PMMNode,
}
// CONTAINS PHYSICAL ADDRESSES
static mut FREELIST: Option<*mut PMMNode> = None;

pub fn init() {
    if let Some(memap_response) = MEMMAP_REQUEST.get_response() {
        for i in memap_response.entries() {
            'a: {
                match i.entry_type {
                    EntryType::USABLE => {
                        unsafe {
                           
                            for i in (i.base..(i.base + i.length)).step_by(Processor::PAGE_SIZE) {
                                if i == 0 {
                                    continue;
                                }

                                if let Some(f) = FREELIST {
                                    (i as *mut PMMNode).byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize).write(PMMNode { next: f });
                                    FREELIST = Some(i as *mut PMMNode);
                                } else {
                                    FREELIST = Some(i as *mut PMMNode);
                                }
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
            println!("giving phy {:x}", phy.addr());
            let bro = phy.byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize);
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
        FREELIST = Some(real.byte_sub(HHDM_REQUEST.get_response().unwrap().offset() as usize));
    }
}
