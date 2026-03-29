use core::ptr::null_mut;

use alloc::{boxed::Box, sync::Arc};
use crate::arch::{context_switch, cpu_local};
use crate::{hcf, kentry, println};
use crate::util::Once;
use crate::{arch::{Arch, Processor}, early_init_pagemap, get_cpu_local, impl_has_list_node, memory::vmm::{Pagemap, VMMFlags}, util::{SpinLock, lists::{ArcInvasiveList, InvasiveListNode}}};
pub struct thread {
    stack_ptr: *mut (),
    timeslice_in_ms: usize,
    next: InvasiveListNode
}
impl_has_list_node!(thread, next);
const STACK_SIZE: usize = 0x40000;
static schedlock: Once<SpinLock> = Once::new();
impl thread {
    fn new(func: impl FnOnce() + 'static + Send, timeslice: usize) -> Result<thread, ()> {
        let stack = early_init_pagemap!().vmm_alloc(STACK_SIZE, VMMFlags::WRITE | VMMFlags::EXECUTABLE).unwrap();
        let stack_used = Processor::prepare_new_thread_stack(unsafe {core::ptr::slice_from_raw_parts_mut::<usize>(stack.cast::<usize>(), STACK_SIZE / size_of::<usize>()).as_mut().unwrap()}, Box::new(func));
        let stack_pt = unsafe {stack.byte_add(STACK_SIZE - stack_used)};
        Ok(thread {
            stack_ptr: stack_pt,
            timeslice_in_ms: timeslice,
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
        Processor::enable_interrupts();
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
    let next = match cpu.run_queue.pop_front() {
        Some(thr) => thr,
        None => {
            let mut idle = None;
            core::mem::swap(&mut idle, &mut cpu.idle_thread);
            idle.expect("no idle thread")
        }
    };
    Processor::set_timer_ms(next.timeslice_in_ms);
    cpu.cur_thread = Some(next);
    let new_stack_ptr = &raw const cpu.cur_thread.as_ref().unwrap().stack_ptr;
    let lock_ptr = schedlock.get().unwrap() as *const SpinLock as *mut ();
    unsafe {context_switch::<*mut ()>(lock_ptr, old_stack_ptr, new_stack_ptr)};
    let ble = unsafe {
        (lock_ptr as *const SpinLock).as_ref().unwrap()
    };
    ble.unlock();

}

pub fn sched_init() {
    println!("starting sched");
    schedlock.call_once(||SpinLock::new());
    let new_loc = get_cpu_local!();

    let thr = thread::new(|| {
            kentry();
            hcf();
    }, 1).unwrap();
    let idle = thread::new(|| {
        loop { core::hint::spin_loop(); }
    }, 10).unwrap();
    unsafe {
        (*new_loc).run_queue.push_back(Arc::new(thr)).unwrap();
        (*new_loc).idle_thread = Some(Arc::new(idle));
    }
    Processor::set_timer_ms(10);
    Processor::enable_interrupts();
    loop {
        hcf();
    }
}