use core::ptr::null_mut;

use alloc::{boxed::Box, sync::Arc};
use crate::arch::{context_switch, cpu_local};
use crate::println;
use crate::util::Once;
use crate::{arch::{Arch, Processor}, early_init_pagemap, get_cpu_local, impl_has_list_node, memory::vmm::{Pagemap, VMMFlags}, util::{SpinLock, lists::{ArcInvasiveList, InvasiveListNode}}};
pub struct thread {
    stack_ptr: *mut (),
    next: InvasiveListNode
}
impl_has_list_node!(thread, next);
const STACK_SIZE: usize = 0x40000;
static schedlock: Once<SpinLock> = Once::new();
impl thread {
    fn new(func: impl FnOnce() + 'static + Send) -> Result<thread, ()> {
        let stack = early_init_pagemap!().vmm_alloc(STACK_SIZE, VMMFlags::WRITE | VMMFlags::EXECUTABLE).unwrap();
        let stack_used = Processor::prepare_new_thread_stack(unsafe {core::ptr::slice_from_raw_parts_mut::<usize>(stack.cast::<usize>(), STACK_SIZE / size_of::<usize>()).as_mut().unwrap()}, Box::new(func));
        let stack_pt = unsafe {stack.byte_add(STACK_SIZE - stack_used)};
        Ok(thread {
            stack_ptr: stack_pt,
            next: InvasiveListNode::new()
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
        let x = unsafe {((pass as *const SpinLock).as_ref().unwrap())};
        x.unlock();
        Box::from_raw(code)();
        panic!("no more");
    }
}
pub fn sched_yield() {
    let cpu = unsafe {get_cpu_local!().as_mut().unwrap()};
    // emma does this and this is kinda smart so i will too
    let mut old_stack = null_mut();
    let mut old_stack_ptr: *mut *mut () = &raw mut old_stack;
    schedlock.get().unwrap().lock();
    if cpu.cur_thread.is_some() {
        let mut old_thr: Option<Arc<thread>> = None;
        core::mem::swap(&mut old_thr, &mut cpu.cur_thread);
        // THIS IS SOME BULLSHITTT but to get around the borrow check clanker told me this is what i should do so fuck it 
        old_stack_ptr = unsafe { &raw mut (*(Arc::as_ptr(old_thr.as_ref().unwrap()) as *mut thread)).stack_ptr };
        cpu.run_queue.push_back(old_thr.unwrap()).unwrap();
    }
    if let Some(new_thr) = cpu.run_queue.pop_front() {
        cpu.cur_thread = Some(new_thr);
        let new_stack_ptr = &raw const cpu.cur_thread.as_ref().unwrap().stack_ptr;
        let lock_ptr = schedlock.get().unwrap() as *const SpinLock as *mut ();
        unsafe {context_switch::<*mut ()>(lock_ptr, old_stack_ptr, new_stack_ptr)};
        let mut ble = unsafe {
            (lock_ptr as *const SpinLock).as_ref().unwrap()
        };
        ble.unlock();
    } else {
        panic!("no thread");
    }

}

pub fn sched_test() {
    println!("doing test");
    schedlock.call_once(||SpinLock::new());
    let new_loc = get_cpu_local!();
    let thr = thread::new(|| {
        loop {
        println!("hello world");
        core::hint::spin_loop();
        sched_yield(); }
    }).unwrap();
    let thr2 = thread::new(|| {
        loop {
        println!("hello world from thread 2");
        core::hint::spin_loop();
        sched_yield(); }
    }).unwrap();
    unsafe {
        
        (*new_loc).run_queue.push_back(Arc::new(thr)).unwrap();
        (*new_loc).run_queue.push_back(Arc::new(thr2)).unwrap();
        (*new_loc).cur_thread = Some(Arc::new(thread {
            stack_ptr: null_mut(),
            next: InvasiveListNode::new(),
        }));
    }
    loop {
        println!("hello world from original code");
        sched_yield();
    }
    println!("came back from thread");
}