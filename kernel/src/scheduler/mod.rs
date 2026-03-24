use alloc::boxed::Box;
use crate::{arch::{Arch, Processor}, early_init_pagemap, memory::vmm::{Pagemap, VMMFlags}, util::SpinLock};

struct thread {
    stack_ptr: *mut (),
}
const STACK_SIZE: usize = 0x40000;
impl thread {
    fn new(func: impl FnOnce() + 'static + Send) -> Result<thread, ()> {
        let stack = early_init_pagemap!().vmm_alloc(STACK_SIZE, VMMFlags::WRITE | VMMFlags::EXECUTABLE).unwrap();
        let stack_used = Processor::prepare_new_thread_stack(unsafe {core::ptr::slice_from_raw_parts_mut::<usize>(stack.cast::<usize>(), STACK_SIZE / size_of::<usize>()).as_mut().unwrap()}, Box::new(func));
        let stack_pt = unsafe {stack.byte_add(STACK_SIZE - stack_used)};
        Ok(thread {
            stack_ptr: stack_pt
        })
    }
}
pub unsafe extern "C" fn sched_tramp2(
    pass: *mut (), // contains whatever you need to unlock the runqueue
    addr: *mut (),
    meta: *mut ()
) {
    unsafe {
        let code: *mut dyn FnOnce() = core::ptr::from_raw_parts_mut(addr, core::mem::transmute(meta));
        Box::from_raw(code)();
        panic!("no more");
    }
}