use core::alloc::GlobalAlloc;


use crate::{memory::{slab::{slab_alloc, slab_dealloc}, vmm::{VMMFlags, kermap}}, util::SpinLock};

pub mod pmm;
pub mod slab;
pub mod vmm;


unsafe impl GlobalAlloc for NAlloc{
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        self.lock.lock();
        if layout.size() <= 1024 {
            let s = slab_alloc(layout.size()).unwrap().cast();
            self.lock.unlock();
            return s;
        } else {
            let s =  unsafe {
                (&mut *core::ptr::addr_of_mut!(kermap)).as_mut().unwrap().vmm_alloc(layout.size(), VMMFlags::WRITE).unwrap().cast()
            };
            self.lock.unlock();
            return s;
        }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        if layout.size() <= 1024 {
            slab_dealloc(ptr.cast());
        } else {
            unsafe {(&mut *core::ptr::addr_of_mut!(kermap)).as_mut().unwrap().vmm_dealloc(ptr.cast(), false)};
        }
    }
}

struct NAlloc {
    lock: SpinLock
}


#[global_allocator]
static ALLOCATER: NAlloc = NAlloc {
    lock: SpinLock::new()
};