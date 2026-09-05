use core::ptr::null_mut;

use alloc::{boxed::Box, sync::Arc};
#[cfg(any(target_arch = "x86_64", target_arch = "riscv64"))]
use limine::request::PagingModeRequest;
use limine::{mp::MpRespData, paging::PagingMode, request::Response};

#[cfg(target_arch = "x86_64")]
use crate::arch::x86_64::{gdt::GdtTable, tss::tss};
use crate::{
    memory::vmm::{Pagemap, VMMFlags},
    scheduler::thread,
    util::{SpinLock, lists::ArcInvasiveList},
};

#[cfg(target_arch = "riscv64")]
pub mod risc_v;
#[cfg(target_arch = "x86_64")]
pub mod x86_64;
#[unsafe(link_section = ".requests")]
#[cfg(any(target_arch = "x86_64", target_arch = "aarch64"))] // x86_64 and AArch64 share the same modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new(
    PagingMode::X86_64_4LVL,
    PagingMode::X86_64_4LVL,
    PagingMode::X86_64_4LVL,
);

#[unsafe(link_section = ".requests")]
#[cfg(target_arch = "riscv64")] // RISC-V has different modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new(
    PagingMode::RISCV_SV48,
    PagingMode::RISCV_SV48,
    PagingMode::RISCV_SV39,
);
#[cfg(target_arch = "riscv64")]
pub static N_CPUREGS: usize = 32; //change dep on arch
#[cfg(target_arch = "x86_64")]
pub static N_CPUREGS: usize = 22; //16 gpr, each xmm is like 2 regs, and there are 32 xmm, so 64
pub trait ArchContext {
    fn instruction_ptr(&self) -> usize;
    fn is_user(&self) -> bool;
}
pub trait Arch {
    type CPUContext: ArchContext;
    const PAGE_SIZE: usize;

    fn arch_bsp_init();
    fn arch_bootstrap(res: &Response<MpRespData>);
    fn get_root_table() -> *mut u64;
    fn pt_init() -> (usize, usize);
    fn raw_io_in(addr: u64, byte_width: u8) -> u64;
    fn raw_io_out(addr: u64, data: u64, byte_width: u8);
    fn init_timer();
    fn set_timer_ms(ms: usize);

    fn init_cpu_local(ptr: *mut cpu_local);
    fn mask_timer();
    fn acknowledge_interrupt();
    fn enable_interrupts();
    fn disable_interrupts();
    fn is_interrupts_enabled() -> bool;
    fn context_switch<T: ArchContext>(old_thread: Option<Arc<thread>>, new_thread: &[usize], frame: &mut T);
    fn Prepare_thread(slice: &mut [usize], entry: usize, stack_ptr: *mut (), user: bool);
    fn set_interrupt_stack(stack_ptr: *mut ()) -> Result<(), &'static str>;
}

pub struct Processor {}

#[repr(C)]
pub struct cpu_local {
    pub sel: *mut cpu_local,
    pub preempt: bool,
    pub run_queue: ArcInvasiveList<thread>,
    pub run_lock: SpinLock,
    pub cur_thread: Option<Arc<thread>>,
    pub idle_thread: Option<Arc<thread>>,
    pub interrupt_stack: *mut (),
    #[cfg(target_arch = "x86_64")]
    pub lapic: Option<x86_64::lapic::lapic>,
    #[cfg(target_arch = "x86_64")]
    pub gdt: Option<GdtTable>, // bsp wont have one
    #[cfg(target_arch = "x86_64")]
    pub tss_ptr: *mut tss,
}

impl cpu_local {
    pub fn new(bsp: bool) -> *mut cpu_local {
        let h = Box::new(cpu_local {
            preempt: true,
            sel: null_mut(),
            run_queue: ArcInvasiveList::new(),
            cur_thread: None,
            idle_thread: None,
            run_lock: SpinLock::new(),
            interrupt_stack: unsafe {
                use crate::scheduler::STACK_SIZE;
                use crate::early_init_pagemap;
                let stack = early_init_pagemap!()
            .vmm_alloc(STACK_SIZE, VMMFlags::WRITE | VMMFlags::EXECUTABLE)
            .unwrap().byte_add(STACK_SIZE);
        stack
            },
            #[cfg(target_arch = "x86_64")]
            lapic: None,
            #[cfg(target_arch = "x86_64")]
            gdt: None,
            #[cfg(target_arch = "x86_64")]
            tss_ptr: null_mut(),
        });

        let l = Box::into_raw(h);
        unsafe {
            (*l).sel = l;

            #[cfg(target_arch = "x86_64")]
            {
                use crate::arch::x86_64::gdt::{BSP_TSS, GDTtss};
                (*l).tss_ptr = if bsp {
                    BSP_TSS.get()
                } else {
                    Box::into_raw(Box::new(tss::new()))
                };
                if !bsp {
                    let mut g = GdtTable::new();
                    g.tss = GDTtss::new((*l).tss_ptr);
                    (*l).gdt = Some(g);
                }
            }

            Processor::init_cpu_local(l);
        }
        l
    }
}
