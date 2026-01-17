use core::fmt::Debug;
use core::slice::Iter;

use bytemuck::{Pod, Zeroable};
use limine::memory_map::EntryType;
use limine::request::MemoryMapRequest;

use crate::arch::{Arch, Processor};
use crate::{HHDM_REQUEST, println};
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
                        if let None = FREELIST {
                            FREELIST = Some(i.base as *mut PMMNode);
                            break 'a;
                        };
                        println!("hex: {:x?}", (i.base, i.length));
                        let mut x = FREELIST.expect("no usable pages lmaoooooooooo");
                        for i in (i.base..(i.base + i.length)).step_by(Processor::PAGE_SIZE) {
                            (*(x.byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize))).next = i as *mut PMMNode;
                            x = i as *mut PMMNode;
                            FREELIST = Some(i as *mut PMMNode);
                        }

                    };
                }
                _ => {}
            } }
        }
    };
    println!("i suppose it worked?? freelist created? {:?}", FREELIST.unwrap());
    
}

pub fn allocate_page<T>() -> *mut T {
    unsafe {
    if let Some(phy) = FREELIST {
        let bro = phy.byte_add(HHDM_REQUEST.get_response().unwrap().offset() as usize);
        bro.write_bytes(0, Processor::PAGE_SIZE);
        return bro as *mut T;
    } else {
        panic!("gbr");
    }};
}

pub fn deallocate_page<T>(ptr: *mut T) {
    unsafe {
      let o = FREELIST.unwrap();
      ptr.write_bytes(0, Processor::PAGE_SIZE);
      let real = ptr as *mut PMMNode;
      (*real).next = o;
      FREELIST = Some(real.byte_sub(HHDM_REQUEST.get_response().unwrap().offset() as usize));  
    }
}