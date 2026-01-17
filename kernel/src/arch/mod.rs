
#[cfg(target_arch = "x86_64")]
pub mod x86_64;
#[cfg(target_arch = "riscv64")]
pub mod risc_v;
pub trait Arch {
    const PAGE_SIZE: usize;
    fn arch_init();
}
pub struct Processor{}
