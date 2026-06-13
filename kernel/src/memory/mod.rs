use core::alloc::GlobalAlloc;

use crate::{
    memory::{
        slab::{slab_alloc, slab_dealloc},
        vmm::{VMMFlags, kermap},
    },
    util::SpinLock,
};

pub mod pmm;
pub mod slab;
pub mod vmm;
impl NAlloc {
    pub unsafe fn alloc(&self, size: usize) -> *mut u8 {
        self.lock.lock();
        if size <= 1024 {
            let s = slab_alloc(size).unwrap().cast();
            self.lock.unlock();
            return s;
        } else {
            let s = unsafe {
                (&mut *core::ptr::addr_of_mut!(kermap))
                    .as_mut()
                    .unwrap()
                    .vmm_alloc(size, VMMFlags::WRITE)
                    .unwrap()
                    .cast()
            };
            self.lock.unlock();
            return s;
        }
    }
    pub unsafe fn dealloc(&self, ptr: *mut u8, size: usize) {
        if size <= 1024 {
            slab_dealloc(ptr.cast());
        } else {
            unsafe {
                (&mut *core::ptr::addr_of_mut!(kermap))
                    .as_mut()
                    .unwrap()
                    .vmm_dealloc(ptr.cast(), false)
            };
        }
    }
}
unsafe impl GlobalAlloc for NAlloc {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        unsafe {self.alloc(layout.size())}
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        unsafe {self.dealloc(ptr, layout.size());}
    }
}

pub struct NAlloc {
    lock: SpinLock,
}

#[global_allocator]
pub static ALLOCATER: NAlloc = NAlloc {
    lock: SpinLock::new(),
};
