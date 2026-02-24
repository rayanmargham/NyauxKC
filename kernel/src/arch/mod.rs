use limine_boot::paging::PagingMode;
#[cfg(any(target_arch = "x86_64", target_arch = "riscv64"))]
use limine_boot::request::PagingModeRequest;

use crate::memory::vmm::{Pagemap, VMMFlags};


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
}
pub struct Processor{}
