use core::{hint, sync::atomic::AtomicBool};


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