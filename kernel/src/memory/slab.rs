use core::error::Error;
use core::ptr::null_mut;

use bytemuck::Pod;
use bytemuck::Zeroable;

use crate::arch::Arch;
use crate::arch::Processor;
use crate::memory::pmm::allocate_page;
use crate::println;

#[derive(Debug)]
pub struct slab_obj {
    next: *mut slab_obj,
}
#[derive(Debug)]
pub struct slab_header {
    obj_am: usize,
    obj_size: usize,
    obj: *mut slab_obj,
    other_slabs: *mut slab_header,
}

#[derive(Copy, Clone, Debug)]
pub struct slabcache {
    size: usize,
    slabs: *mut slab_header,
}
impl slab_header {
    fn init(size: usize) -> *mut slab_header {
        let page = allocate_page();
        let obj_am = (Processor::PAGE_SIZE - size_of::<slab_header>()) / size;
        let slab_heade: *mut slab_header = page.cast();
        let mut obj: *mut slab_obj = unsafe { (page as *mut slab_header).add(1).cast() };

       
        for i in 1..obj_am {
            let new_obj: *mut slab_obj =
                unsafe { (page as *mut slab_header).add(1).byte_add(i * size).cast() };
            unsafe {
                (*new_obj).next = obj;
                obj = new_obj;
            }
        }
     
        unsafe {slab_heade.write(slab_header { obj_am, obj_size: size, obj, other_slabs: core::ptr::null_mut() })};
        return slab_heade;
    }
    fn alloc(&mut self) -> Result<*mut (), ()> {
        let mut cur_slab = self;
        println!("obj size: {}", cur_slab.obj_size);
        loop {
            if cur_slab.obj != core::ptr::null_mut() {
                let bro = unsafe { (*cur_slab.obj).next };
                if bro != core::ptr::null_mut() {
                    unsafe {
                        (*cur_slab.obj).next = (*bro).next;

                        bro.cast::<u8>().write_bytes(0, cur_slab.obj_size);
                    };
                    return Ok(bro.cast());
                } else {
                    unsafe {
                        cur_slab.obj.cast::<u8>().write_bytes(0, cur_slab.obj_size);
                    };
                    return Ok(cur_slab.obj.cast());
                }
            }
            unsafe {
                if let Some(b) = cur_slab.other_slabs.as_mut() {
                    cur_slab = b;
                } else {
                    cur_slab = slab_header::init(cur_slab.obj_size).as_mut().unwrap();
                }
            };
        }
        Err(())
    }

}
impl slabcache {
    fn init(size: usize) -> Result<slabcache, ()> {
        if size_of::<slab_header>() + size > Processor::PAGE_SIZE {
            return Err(());
        }
        let h = slab_header::init(size);

        println!("{:?}", h.read());

        Ok(slabcache { size, slabs: h })
    }
    fn alloc(&mut self) -> Result<*mut (), ()> {
        if self.slabs != core::ptr::null_mut() {
            println!("{:?}", self.slabs.read());
            return unsafe {(*self.slabs).alloc()};
        } else {
            panic!("slab cache not inited yet");
        }
    }

}
static mut slab_caches: [slabcache; 7] = [slabcache {
    size: 0,
    slabs: null_mut(),
}; 7];
// @brief PMM must be initted before this function is ran or your not sigma alpha sigma alpha sigma alpha
pub fn init_slab() {
    println!("initing slab");
    unsafe {
        slab_caches[0] = slabcache::init(16).unwrap();
        slab_caches[1] = slabcache::init(32).unwrap();
        slab_caches[2] = slabcache::init(64).unwrap();
        slab_caches[3] = slabcache::init(128).unwrap();
        slab_caches[4] = slabcache::init(256).unwrap();
        slab_caches[5] = slabcache::init(512).unwrap();
        slab_caches[6] = slabcache::init(1024).unwrap();
    }
    println!("slab caches inited");
}

pub fn slab_alloc(size: usize) -> Result<*mut (), ()> {
    assert!(size <= size.next_power_of_two());
    for i in 0..6 {
        let mut s = unsafe {slab_caches[i]};
        println!("index {}, size {}", i, s.size);
        if s.size < size {
            continue;
        }
        return s.alloc();
    }
    Err(())
}
pub fn slab_dealloc(addr: *mut ()) {
    if addr == core::ptr::null_mut() {
            return;
        }
       let hehe = addr.map_addr(|a| a & !0xFFF).cast() as *mut slab_header;
        let sizeofobj = unsafe { (*hehe).obj_size };
        unsafe { addr.cast::<u8>().write_bytes(0, sizeofobj) };
        let old = unsafe { (*hehe).obj };
        if old == core::ptr::null_mut() {
            unsafe {
                (*hehe).obj = addr.cast();
            }
        } else {
            unsafe {
                (*(addr.cast() as *mut slab_obj)).next = old;
                (*hehe).obj = addr.cast();
            }
        }
}