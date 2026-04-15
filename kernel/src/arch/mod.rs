use core::ptr::null_mut;

use alloc::{boxed::Box, sync::Arc};
use limine_boot::paging::PagingMode;
#[cfg(any(target_arch = "x86_64", target_arch = "riscv64"))]
use limine_boot::request::PagingModeRequest;

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
pub trait Arch {
    const PAGE_SIZE: usize;
    fn arch_init();
    fn get_root_table() -> *mut u64;
    fn pt_init() -> (usize, usize);
    fn raw_io_in(addr: u64, byte_width: u8) -> u64;
    fn raw_io_out(addr: u64, data: u64, byte_width: u8);
    fn init_timer();
    fn set_timer_ms(ms: usize);
    fn prepare_new_thread_stack(
        stack_ptr: &mut [usize],
        function: Box<dyn FnOnce() + 'static + Send>,
    ) -> usize; // returns how much stack in bytes its used
    fn init_cpu_local(ptr: *mut cpu_local);
    fn mask_timer();
    fn acknowledge_interrupt();
    fn enable_interrupts();
    fn disable_interrupts();
    fn set_interrupt_stack(stack_ptr: *mut ()) -> Result<(), &'static str>;
}
pub struct Processor {}

#[cfg(target_arch = "riscv64")]
pub use risc_v::context_switch;
#[cfg(target_arch = "x86_64")]
pub use x86_64::context_switch;

#[repr(C)]
pub struct cpu_local {
    pub sel: *mut cpu_local,
    pub preempt: bool,
    pub run_queue: ArcInvasiveList<thread>,
    pub run_lock: SpinLock,
    pub cur_thread: Option<Arc<thread>>,
    pub idle_thread: Option<Arc<thread>>,
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
                (*l).tss_ptr = if bsp { BSP_TSS.get() } else { Box::into_raw(Box::new(tss::new())) };
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
