use core::ops::{Deref, DerefMut};
use core::{hint, sync::atomic::AtomicBool};
pub mod lists;
#[macro_export]
macro_rules! early_init_pagemap {
    () => {
        unsafe {
            (&mut *core::ptr::addr_of_mut!(crate::memory::vmm::kermap))
                .as_mut()
                .unwrap()
        }
    };
}

pub struct SpinLock(AtomicBool);
pub struct SpinLockB<T>(SpinLock, UnsafeCell<T>);
pub struct SpinLockBGuard<'a, T> {
    l: &'a SpinLock,
    d: &'a mut T,
}
impl<T> SpinLockB<T> {
    pub const fn new(f: T) -> Self {
        Self(SpinLock::new(), UnsafeCell::new(f))
    }
    pub fn lock<'a>(&'a self) -> SpinLockBGuard<'a, T> {
        self.0.lock();
        SpinLockBGuard {
            l: &self.0,
            d: unsafe { self.1.as_mut_unchecked() },
        }
    }
}
impl<T> Deref for SpinLockBGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        self.d
    }
}
impl<T> DerefMut for SpinLockBGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.d
    }
}
impl<T> Drop for SpinLockBGuard<'_, T> {
    fn drop(&mut self) {
        self.l.unlock();
    }
}
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
use core::cell::UnsafeCell;
use core::sync::atomic::{AtomicU8, Ordering};

use nyaux_uacpi_bindings::{uacpi_char, uacpi_table, uacpi_table_find_by_signature};

use crate::HHDM_REQUEST;
use crate::uacpi::check_ustatus;

const UNINIT: u8 = 0;
const RUNNING: u8 = 1;
const DONE: u8 = 2;
// SYNCCELL IS CLANKER CODE BUT I KNOW WHAT ITS DOING
#[repr(transparent)]
pub struct SyncCell<T>(pub UnsafeCell<T>);
unsafe impl<T> Sync for SyncCell<T> {}

impl<T> SyncCell<T> {
    pub const fn new(val: T) -> Self {
        Self(UnsafeCell::new(val))
    }
    pub fn get(&self) -> *mut T {
        self.0.get()
    }
    pub fn get_mut_unchecked(&self) -> &mut T {
        unsafe { self.0.get().as_mut().unwrap() }
    }
}

pub struct Once<T> {
    state: AtomicU8,
    data: UnsafeCell<Option<T>>,
}
unsafe impl<T> Send for SpinLockB<T>{}
unsafe impl<T> Sync for SpinLockB<T>{}
unsafe impl<T: Send + Sync> Sync for Once<T> {}

impl<T> Once<T> {
    pub const fn new() -> Self {
        Self {
            state: AtomicU8::new(UNINIT),
            data: UnsafeCell::new(None),
        }
    }

    pub fn call_once(&self, f: impl FnOnce() -> T) {
        // Try to be the one who initializes
        if self
            .state
            .compare_exchange(UNINIT, RUNNING, Ordering::Acquire, Ordering::Acquire)
            .is_ok()
        {
            unsafe {
                *self.data.get() = Some(f());
            }
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
pub fn find_acpi_table(tabl: *const uacpi_char) -> Result<uacpi_table, &'static str> {
    let mut table: uacpi_table = unsafe { core::mem::zeroed() };
    let status = unsafe { uacpi_table_find_by_signature(tabl, &mut table) };
    let hm = check_ustatus(status);
    if hm.is_err() {
        return Err(hm.err().unwrap());
    }
    return Ok(table);
}
pub fn to_hhdm<T>(ptr: *mut T) -> *mut T {
    unsafe { ptr.byte_add(HHDM_REQUEST.response().unwrap().offset as usize) }
}
