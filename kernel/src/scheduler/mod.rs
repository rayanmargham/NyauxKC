use core::ptr::null_mut;

use crate::arch::{ArchContext, N_CPUREGS, cpu_local};
use crate::util::Once;
use crate::{
    arch::{Arch, Processor},
    early_init_pagemap, get_cpu_local, impl_has_list_node,
    memory::vmm::{Pagemap, VMMFlags},
    util::{
        SpinLock,
        lists::{ArcInvasiveList, InvasiveListNode},
    },
};
use crate::{HHDM_REQUEST, hcf, kentry, println};
use alloc::{boxed::Box, sync::Arc};
pub struct thread {
    stack_ptr: *mut (),
    pub cpucontext: [usize; N_CPUREGS],
    timeslice_in_ms: usize,
    next: InvasiveListNode,
}
impl_has_list_node!(thread, next);
const LIMINE_STACK_SIZE: usize = 65536;
pub const STACK_SIZE: usize = 0x40000;
static schedlock: Once<SpinLock> = Once::new();
impl thread {
    fn new(func: usize, timeslice: usize, user: bool) -> Result<thread, ()> {
        let stack = early_init_pagemap!()
            .vmm_alloc(STACK_SIZE, VMMFlags::WRITE | VMMFlags::EXECUTABLE)
            .unwrap();
        let mut cpuctx: [usize; N_CPUREGS] = [0; N_CPUREGS];
        let stack_pt = unsafe { stack.byte_add(STACK_SIZE) };
        Processor::Prepare_thread(&mut cpuctx, func as usize, stack_pt, user);
        
        Ok(thread {
            stack_ptr: stack_pt,
            cpucontext: cpuctx,
            timeslice_in_ms: timeslice,
            next: InvasiveListNode::new(),

        })
    }

    fn current() -> Option<*const thread> {
        let c = unsafe {
            get_cpu_local!().as_ref().unwrap()
        };
        c.cur_thread.as_ref().and_then(|f|Some(f.as_ref() as *const thread))
    }
}
// pub unsafe extern "C" fn sched_tramp2(
//     pass: *mut (), // contains whatever you need to unlock the runqueue
//     addr: *mut (),
//     meta: *mut (),
// ) {
//     unsafe {
//         let code: *mut dyn FnOnce() =
//             core::ptr::from_raw_parts_mut(addr, core::mem::transmute(meta));
//         let x = unsafe { ((pass as *const SpinLock).as_ref().unwrap()) };
//         x.unlock();
//         Processor::enable_interrupts();
//         Box::from_raw(code)();
//         panic!("no more");
//     }
// }
pub fn sched_yield<T: ArchContext>(cpuframe: &mut T) {
    let cpu = unsafe { get_cpu_local!().as_mut().unwrap() };
    // emma does this and this is kinda smart so i will too
    let mut oldregs = None;
    schedlock.get().unwrap().lock();
    if cpu.cur_thread.is_some() {
        let mut old_thr: Option<Arc<thread>> = None;
        core::mem::swap(&mut old_thr, &mut cpu.cur_thread);
        // THIS IS SOME BULLSHITTT but to get around the borrow check clanker told me this is what i should do so fuck it
        let a = old_thr.as_ref().unwrap().clone();
        oldregs = Some(a);
        cpu.run_queue.push_back(old_thr.unwrap()).unwrap();
    }
    let next = match cpu.run_queue.pop_front() {
        Some(thr) => thr,
        None => {
            let mut idle = None;
            core::mem::swap(&mut idle, &mut cpu.idle_thread);
            idle.expect("no idle thread")
        }
    }
    ;
    Processor::context_switch(oldregs, &next.cpucontext,cpuframe);
    Processor::set_timer_ms(next.timeslice_in_ms);

    cpu.cur_thread = Some(next);
    schedlock.get().unwrap().unlock();
}
extern "C" fn useless() {
loop {
                core::hint::spin_loop();
            }
}
pub fn sched_init() {
    println!("starting sched");
    schedlock.call_once(|| SpinLock::new());
    let new_loc = get_cpu_local!();



    let idle = thread::new(
        useless as usize,
        10,
        false
    )
    .unwrap();
    unsafe {
        (*new_loc).run_queue.push_back(Arc::new(

            thread::new(kentry as usize, 1, false).unwrap()
        )).unwrap();
        (*new_loc).idle_thread = Some(Arc::new(idle ));
    }
    Processor::set_timer_ms(10);
    Processor::enable_interrupts();
    loop {
        hcf();
    }
}
