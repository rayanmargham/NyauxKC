#[cfg(any(target_arch = "x86_64", target_arch = "aarch64"))]
use limine::paging;
use limine::request::PagingModeRequest;


#[cfg(target_arch = "x86_64")]
pub mod x86_64;
#[cfg(target_arch = "riscv64")]
pub mod risc_v;
#[unsafe(link_section = ".requests")]
#[cfg(any(target_arch = "x86_64", target_arch = "aarch64"))] // x86_64 and AArch64 share the same modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new().with_mode(paging::Mode::FOUR_LEVEL);
#[unsafe(link_section = ".requests")]
#[cfg(target_arch = "riscv64")] // RISC-V has different modes
static PAGING_MODE_REQUEST: PagingModeRequest = PagingModeRequest::new().with_mode(paging::Mode::SV48);
pub trait Arch {
    const PAGE_SIZE: usize;
    fn arch_init();
}
pub struct Processor{}
