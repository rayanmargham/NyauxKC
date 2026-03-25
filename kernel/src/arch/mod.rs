use alloc::boxed::Box;
use limine_boot::paging::PagingMode;
#[cfg(any(target_arch = "x86_64", target_arch = "riscv64"))]
use limine_boot::request::PagingModeRequest;

use crate::{memory::vmm::{Pagemap, VMMFlags}, timers::CalibrationTimer};


#[cfg(target_arch = "x86_64")]
pub mod x86_64;
#[cfg(target_arch = "riscv64")]
pub mod risc_v;
#[unsafe(link_section = ".requests")]
#[cfg(any(target_arch = "x86_64", target_arch = "aarch64"))] // x86_64 and AArch64 share the same modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new(PagingMode::X86_64_4LVL, PagingMode::X86_64_4LVL, PagingMode::X86_64_4LVL);
#[unsafe(link_section = ".requests")]
#[cfg(target_arch = "riscv64")] // RISC-V has different modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new(PagingMode::RISCV_SV48, PagingMode::RISCV_SV48, PagingMode::RISCV_SV39);
pub trait Arch {
    const PAGE_SIZE: usize;
    fn arch_init();
    fn get_root_table() -> *mut u64;
    fn pt_init() -> (usize, usize);
    fn raw_io_in(addr: u64, byte_width: u8) -> u64;
    fn raw_io_out(addr: u64, data: u64, byte_width: u8);
    fn calibrate_preemption_timer(calibrator: Box<dyn CalibrationTimer>);
    fn prepare_new_thread_stack(stack_ptr: &mut [usize], function: Box<dyn FnOnce() + 'static + Send>) -> usize; // returns how much stack in bytes its used
    fn init_cpu_local(ptr: *mut cpu_local);
}
pub struct Processor{}

pub struct cpu_local {
    test: usize
}