use core::ops::{Deref, DerefMut};
use core::sync::atomic::Ordering::{Acquire, Relaxed, Release};
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

// CLANKER CODE
// auditted because I am not great at atomics, but it makes sense to me what this clanker code is doing
use core::cell::UnsafeCell;
use core::sync::atomic::{AtomicU8, AtomicU32, Ordering};

use nyaux_uacpi_bindings::{uacpi_char, uacpi_table, uacpi_table_find_by_signature};

use crate::HHDM_REQUEST;
use crate::arch::{Arch, Processor};
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
unsafe impl<T> Send for SpinLockB<T> {}
unsafe impl<T> Sync for SpinLockB<T> {}
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
pub struct SpinLock(AtomicBool, AtomicBool);
pub struct RWSpinLock<T>(pub AtomicU32, pub UnsafeCell<T>);
pub struct SpinReadGuard<'a, T> {
    pub lock: &'a RWSpinLock<T>,
    // CLANKERCODE
    prev: bool
    //CLANKERCODE
}
pub struct SpinWriteGuard<'a, T> {
    pub lock: &'a RWSpinLock<T>,
    // CLANKERCODE
    prev: bool
    // CLANKERCODE
}
impl<T> Deref for SpinWriteGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        unsafe { self.lock.1.as_ref_unchecked() }
    }
}
impl<T> DerefMut for SpinWriteGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { self.lock.1.as_mut_unchecked() }
    }
}

impl<T> Deref for SpinReadGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        unsafe { self.lock.1.as_ref_unchecked() }
        
    }
}
impl<T> Drop for SpinReadGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.0.fetch_sub(1, Release);
        // CLANKER CODE
        if self.prev { Processor::enable_interrupts(); }
        // CLANKERCODE
    }
}
impl<T> Drop for SpinWriteGuard<'_, T> {
    fn drop(&mut self) {
        self.lock.0.store(0, Release);
        // CLANKERCODE
        if self.prev { Processor::enable_interrupts(); }
        // CLANKERCODE
    }
}
unsafe impl<T: Send> Send for RWSpinLock<T> {}
unsafe impl<T: Send + Sync> Sync for RWSpinLock<T> {}
impl<T> RWSpinLock<T> {
    pub const fn new(d: T) -> Self {
        Self(AtomicU32::new(0), UnsafeCell::new(d))
    }
    // CLANKERCODE I DONT UNDERSTAND WHAT IT FIXED WITH MY ORIGINAL HUMAN MADE FUNCTION BUT IM WAY TOO UNMOTIVATED TO TRY AND
    // UNDERSTAND THIS ATM IM REALLY TIRED
    // ILL GET AROUND TO IT ONE DAY
    pub fn lock_read(&self) -> SpinReadGuard<'_, T> {
        let prev = Processor::is_interrupts_enabled();
        Processor::disable_interrupts();
        let mut spun = false;
        loop {
            if spun {
                if prev {
                    Processor::enable_interrupts();
                }
                hint::spin_loop();
                Processor::disable_interrupts();
            }
            spun = true;

            let old = self.0.load(Relaxed);
            if old >= u32::MAX - 1 {
                continue; // someone is writing lol
            }
            if self
                .0
                .compare_exchange_weak(old, old + 1, Acquire, Relaxed)
                .is_ok()
            {
                return SpinReadGuard { lock: self, prev };
            }
        }
    }

    pub fn lock_write(&self) -> SpinWriteGuard<'_, T> {
        let prev = Processor::is_interrupts_enabled();
        Processor::disable_interrupts();
        let mut spun = false;
        loop {
            if spun {
                if prev {
                    Processor::enable_interrupts();
                }
                hint::spin_loop();
                Processor::disable_interrupts();
            }
            spun = true;

            if self
                .0
                .compare_exchange_weak(0, u32::MAX, Acquire, Relaxed)
                .is_ok()
            {
                return SpinWriteGuard { lock: self, prev };
            }
        }
    }
    // CLANKERCODE
}
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
        Self(AtomicBool::new(false), AtomicBool::new(false))
    }
    #[inline]
    pub fn lock(&self) {
        let prev = Processor::is_interrupts_enabled();
        Processor::disable_interrupts();
        while self.0.swap(true, core::sync::atomic::Ordering::Acquire) {
            if (prev) {
                Processor::enable_interrupts();
            }
            hint::spin_loop();
            Processor::disable_interrupts();
        }
        self.1.store(prev, Ordering::Relaxed);
    }
    #[inline]
    pub fn unlock(&self) {
        let prev = self.1.load(Ordering::Relaxed);
        self.0.store(false, core::sync::atomic::Ordering::Release);
        if prev {
            Processor::enable_interrupts();
        }
    }
}
#[derive(Debug)]
#[repr(isize)]
pub enum errno {
    EPERM = 1,
    ENOENT = 2,
    ESRCH = 3,
    EINTR = 4,
    EIO = 5,
    ENXIO = 6,
    E2BIG = 7,
    ENOEXEC = 8,
    EBADF = 9,
    ECHILD = 10,
    EAGAIN = 11,
    ENOMEM = 12,
    EACCES = 13,
    EFAULT = 14,
    ENOTBLK = 15,
    EBUSY = 16,
    EEXIST = 17,
    EXDEV = 18,
    ENODEV = 19,
    ENOTDIR = 20,
    EISDIR = 21,
    EINVAL = 22,
    ENFILE = 23,
    EMFILE = 24,
    ENOTTY = 25,
    ETXTBSY = 26,
    EFBIG = 27,
    ENOSPC = 28,
    ESPIPE = 29,
    EROFS = 30,
    EMLINK = 31,
    EPIPE = 32,
    EDOM = 33,
    ERANGE = 34,
    ENAMETOOLONG = 36,
    ENOLCK = 37,
    ENOSYS = 38,
    ENOTEMPTY = 39,
    ELOOP = 40,
    ENOMSG = 42,
    EIDRM = 43,
    ECHRNG = 44,
    EL2NSYNC = 45,
    EL3HLT = 46,
    EL3RST = 47,
    ELNRNG = 48,
    EUNATCH = 49,
    ENOCSI = 50,
    EL2HLT = 51,
    EBADE = 52,
    EBADR = 53,
    EXFULL = 54,
    ENOANO = 55,
    EBADRQC = 56,
    EBADSLT = 57,
    EDEADLOCK = 35,
    EBFONT = 59,
    ENOSTR = 60,
    ENODATA = 61,
    ETIME = 62,
    ENOSR = 63,
    ENONET = 64,
    ENOPKG = 65,
    EREMOTE = 66,
    ENOLINK = 67,
    EADV = 68,
    ESRMNT = 69,
    ECOMM = 70,
    EPROTO = 71,
    EMULTIHOP = 72,
    EDOTDOT = 73,
    EBADMSG = 74,
    EOVERFLOW = 75,
    ENOTUNIQ = 76,
    EBADFD = 77,
    EREMCHG = 78,
    ELIBACC = 79,
    ELIBBAD = 80,
    ELIBSCN = 81,
    ELIBMAX = 82,
    ELIBEXEC = 83,
    EILSEQ = 84,
    ERESTART = 85,
    ESTRPIPE = 86,
    EUSERS = 87,
    ENOTSOCK = 88,
    EDESTADDRREQ = 89,
    EMSGSIZE = 90,
    EPROTOTYPE = 91,
    ENOPROTOOPT = 92,
    EPROTONOSUPPORT = 93,
    ESOCKTNOSUPPORT = 94,
    EOPNOTSUPP = 95,
    EPFNOSUPPORT = 96,
    EAFNOSUPPORT = 97,
    EADDRINUSE = 98,
    EADDRNOTAVAIL = 99,
    ENETDOWN = 100,
    ENETUNREACH = 101,
    ENETRESET = 102,
    ECONNABORTED = 103,
    ECONNRESET = 104,
    ENOBUFS = 105,
    EISCONN = 106,
    ENOTCONN = 107,
    ESHUTDOWN = 108,
    ETOOMANYREFS = 109,
    ETIMEDOUT = 110,
    ECONNREFUSED = 111,
    EHOSTDOWN = 112,
    EHOSTUNREACH = 113,
    EALREADY = 114,
    EINPROGRESS = 115,
    ESTALE = 116,
    EUCLEAN = 117,
    ENOTNAM = 118,
    ENAVAIL = 119,
    EISNAM = 120,
    EREMOTEIO = 121,
    EDQUOT = 122,
    ENOMEDIUM = 123,
    EMEDIUMTYPE = 124,
    ECANCELED = 125,
    ENOKEY = 126,
    EKEYEXPIRED = 127,
    EKEYREVOKED = 128,
    EKEYREJECTED = 129,
    EOWNERDEAD = 130,
    ENOTRECOVERABLE = 131,
    ERFKILL = 132,
    EHWPOISON = 133,
}
pub type EResult<T> = Result<T, errno>;
pub fn grab_cmdline(r: &limine::request::ExecutableCmdlineRequest) -> &str {
    r.response().unwrap().cmdline()
}
#[macro_export]
macro_rules! cmd {
    () => {{
        use crate::CMDLINE_REQUEST;
        use crate::util::grab_cmdline;
        grab_cmdline(&CMDLINE_REQUEST)
    }};
}
