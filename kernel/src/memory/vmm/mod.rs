use bitflags::bitflags;

use crate::{align_up, arch::{Arch, Processor}, memory::slab::slab_alloc};


bitflags! {
    #[derive(PartialEq, Clone, Copy)]
    pub struct VMMFlags: u8 {
        const WRITE = 1 << 0;
        const EXECUTABLE = 1 << 1;
        const GLOBAL = 1 << 2;
        const USER = 1 << 3;
    }
}



pub struct VMMRegion {
    next: Option<*mut VMMRegion>,
    base: usize,
    length: usize,
    flags: VMMFlags
}


impl VMMRegion {
    pub fn new(base: usize, length: usize, flags: VMMFlags) -> *mut VMMRegion {
        let ne = slab_alloc(size_of::<VMMRegion>()).unwrap().cast::<VMMRegion>();
        unsafe {
            ne.write(
                VMMRegion { next: None, base, length, flags }
            );
        }
        ne
    }
}
pub struct Pagemap {
    pub arch_page: *mut u64,
    khead: *mut VMMRegion,
    uhead: *mut VMMRegion
}
pub fn vmm_alloc(pagemap: Pagemap, amount: usize, flags: VMMFlags) -> Result<*mut (), &'static str> {
    if flags.contains(VMMFlags::USER) {
        panic!("todo")
    } else {
        let mut cur: Option<*mut VMMRegion> = Some(pagemap.khead);
        let mut prev: Option<*mut VMMRegion> = None;
        while cur.is_some() {
            if (prev.is_none()) {
                prev = cur;
                cur = unsafe {cur.unwrap().read().next};
                continue;
            }
            let w = unsafe {prev.unwrap().read()};
            let q = unsafe {cur.unwrap().read()};
            if (q.base - (w.base + w.length)) >= (align_up(amount as u64, Processor::PAGE_SIZE as u64) as usize + Processor::PAGE_SIZE as usize) {
                let e = VMMRegion::new((w.base + w.length), align_up(amount as u64, Processor::PAGE_SIZE as u64) as usize, flags);
                unsafe {
                    (*prev.unwrap()).next = Some(e); 
                }
                unsafe {
                    (*e).next = Some(cur.unwrap());
                }
                let info = unsafe {e.read()};
                Processor::arch_map_region(pagemap, info.base, info.length, flags);
                return Ok(core::ptr::without_provenance_mut::<()>(info.base));
            } else {
                prev = cur;
                cur = unsafe {cur.unwrap().read().next};
                continue;
            }
        }
        panic!("gg");
    }
}