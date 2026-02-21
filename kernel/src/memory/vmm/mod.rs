use bitflags::bitflags;

use crate::{HHDM_REQUEST, align_up, arch::{Arch, Processor, x86_64::pt::{KERNELADDR_REQUEST, pt_init}}, memory::slab::{slab_alloc, slab_dealloc}, println};


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
    khead: Option<*mut VMMRegion>,
    uhead: Option<*mut VMMRegion>
}
pub static mut kermap: Option<Pagemap> = None;
impl Pagemap {
    pub fn vmm_alloc(&self, amount: usize, flags: VMMFlags) -> Result<*mut (), &'static str> {
    if flags.contains(VMMFlags::USER) {
        panic!("todo")
    } else {
        let mut cur: Option<*mut VMMRegion> = self.khead;
        let mut prev: Option<*mut VMMRegion> = None;
        while cur.is_some() {
            if prev.is_none() {
                prev = cur;
                cur = unsafe {cur.unwrap().read().next};
                continue;
            }
            let w = unsafe {prev.unwrap().read()};
            let q = unsafe {cur.unwrap().read()};
            let calculation = q.base.saturating_sub(w.base + w.length);
            if calculation >= (align_up(amount as u64, Processor::PAGE_SIZE as u64) as usize + Processor::PAGE_SIZE as usize) {
                let e = VMMRegion::new(w.base + w.length, align_up(amount as u64, Processor::PAGE_SIZE as u64) as usize, flags);
                unsafe {
                    (*prev.unwrap()).next = Some(e); 
                }
                unsafe {
                    (*e).next = Some(cur.unwrap());
                }
                let info = unsafe {e.read()};
                self.arch_map_region(info.base, info.length, flags);
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
pub fn vmm_dealloc(&mut self, base: *mut (), user_allocated: bool) {
    if base.is_null() {
        return;
    }
    if !user_allocated {
        let mut prev: Option<*mut VMMRegion> = None;
        let mut cur = self.khead;

        while cur.is_some() {
            let current = cur.unwrap();
            if unsafe {
                current.read().base
            } == base.addr() {
                // do unmap and dealloc
                let info = unsafe {current.read()};
                self.arch_unmap_region(info.base, info.length);
                if let Some(yo) = prev {
                    unsafe {
                        (*yo).next = 
                            (*current).next;
                        
                    };
                } else {
                    self.khead = unsafe { (*current).next};
                }
                slab_dealloc(current.cast());
                return;
                
            } else {
                prev = cur;
                cur = unsafe {
                    current.read().next
                };
                continue;
            }
        }
    } else {
        panic!("todo");
    }
}
}

pub fn vmm_init() {
    let shit= pt_init();
    let kernel_region = VMMRegion::new(KERNELADDR_REQUEST.get_response().unwrap().virtual_base() as usize,shit.1, VMMFlags::GLOBAL | VMMFlags::EXECUTABLE);
    let hhdm_region = VMMRegion::new(HHDM_REQUEST.get_response().unwrap().offset() as usize, shit.0, VMMFlags::GLOBAL | VMMFlags::EXECUTABLE | VMMFlags::WRITE);
    unsafe {
        (*hhdm_region).next = Some(kernel_region);
    }
    println!("hhdm in pages 0x{:x}", shit.0 / 4096);
    unsafe {
        kermap = Some(
            Pagemap {
                arch_page: Processor::get_root_table(),
                khead: Some(hhdm_region),
                uhead: None,
            }
        )
    }

}