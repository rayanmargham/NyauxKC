use core::error::Error;
use core::ptr::null_mut;

use bytemuck::Pod;
use bytemuck::Zeroable;

use crate::arch::Arch;
use crate::arch::Processor;
use crate::memory::pmm::allocate_page;
use crate::println;


pub struct slab_obj {
    next: *mut slab_obj
}

pub struct slab_header {
    sizeperobj: usize,
    obj: *mut slab_obj
}


#[derive(Copy, Clone)]
pub struct slabcache {
    size: usize,
    slabs: *mut slab_header
}

impl slabcache {
    fn init(size: usize) -> Result<slabcache, ()>{
        if size_of::<slab_header>() > size {
            return Err(());
        }
        let page = allocate_page();
        println!("page allocated");
        let obj_am = (Processor::PAGE_SIZE - size_of::<slab_header>()) / size;

        let slab_heade: *mut slab_header = page.cast();
        let mut obj: *mut slab_obj = unsafe {(page as *mut slab_header).add(1).cast()};

        unsafe {
            (*slab_heade).sizeperobj = obj_am;
            
        }
        println!("okay");
        for i in 1..size {
            let new_obj: *mut slab_obj = unsafe {
                (page as *mut slab_header).add(1).byte_add(i * size).cast()
            };
            unsafe {
                (*new_obj).next = obj;
                obj = new_obj;
            }
        }
        unsafe {
            (*slab_heade).obj = obj;
        }
        println!("done");
        Ok(slabcache { size, slabs: slab_heade })

    }
}
static mut slab_caches: [slabcache; 7] = [slabcache {size: 0, slabs: null_mut()}; 7];
// @brief PMM must be initted before this function is ran or your not sigma alpha sigma alpha sigma alpha
pub fn init_slab() {
    println!("initing slab");
    unsafe {
        slab_caches[0] = slabcache::init(16).unwrap();
        slab_caches[0] = slabcache::init(32).unwrap();
        slab_caches[0] = slabcache::init(64).unwrap();
        slab_caches[0] = slabcache::init(128).unwrap();
        slab_caches[0] = slabcache::init(256).unwrap();
        slab_caches[0] = slabcache::init(512).unwrap();
        slab_caches[0] = slabcache::init(1024).unwrap();







    }
    println!(
        "slab caches inited"
    );
}