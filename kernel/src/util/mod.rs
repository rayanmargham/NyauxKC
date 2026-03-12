use core::{hint, sync::atomic::AtomicBool};

#[macro_export]
macro_rules! early_init_pagemap {
    () => {
        unsafe { (&mut *core::ptr::addr_of_mut!(crate::memory::vmm::kermap)).as_mut().unwrap() }
    };
}


pub struct SpinLock(AtomicBool);

impl SpinLock {
    pub const fn new() -> Self {
        Self(AtomicBool::new(false))
    }
    #[inline]
    pub fn lock(&self) {
        while self.0.swap(true, core::sync::atomic::Ordering::Acquire) {
            hint::spin_loop();
        }
    }
    #[inline]
    pub fn unlock(&self) {
        self.0.store(false, core::sync::atomic::Ordering::Release);
    }
}
// CLANKER CODE
// auditted because I am not great at atomics, but it makes sense to me what this clanker code is doing
use core::sync::atomic::{AtomicU8, Ordering};
use core::cell::UnsafeCell;

const UNINIT: u8 = 0;
const RUNNING: u8 = 1;
const DONE: u8 = 2;

pub struct Once<T> {
    state: AtomicU8,
    data: UnsafeCell<Option<T>>,
}

unsafe impl<T: Send + Sync> Sync for Once<T> {}

impl<T> Once<T> {
    pub const fn new() -> Self {
        Self { state: AtomicU8::new(UNINIT), data: UnsafeCell::new(None) }
    }

    pub fn call_once(&self, f: impl FnOnce() -> T) {
        // Try to be the one who initializes
        if self.state.compare_exchange(UNINIT, RUNNING, Ordering::Acquire, Ordering::Acquire).is_ok() {
            unsafe { *self.data.get() = Some(f()); }
            self.state.store(DONE, Ordering::Release);
        } else {
            // Spin until done (another core is initializing)
            while self.state.load(Ordering::Acquire) != DONE {
                core::hint::spin_loop();
            }
        }
    }

    pub fn get(&self) -> Option<&T> {
        if self.state.load(Ordering::Acquire) == DONE {
            unsafe { (*self.data.get()).as_ref() }
        } else {
            None
        }
    }

}
