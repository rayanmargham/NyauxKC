

use crate::arch::Processor;
use crate::arch::Arch;
use crate::arch::risc_v::interrupts::context_sxr;
use crate::arch::risc_v::interrupts::setup_interrupts;
use crate::println;
#[cfg(target_arch = "riscv64")]
pub mod interrupts;
impl Arch for Processor{
    const PAGE_SIZE: usize = 4096;
    fn arch_init(){
        use crate::memory::pmm;

        println!("RISCV64 init");
        unsafe {
            let meow: u64;
            core::arch::asm!("csrr {}, sstatus", out(reg) meow);
            println!("got {:b}", meow);
            setup_interrupts();
        }
        pmm::init();
        unsafe {
            core::ptr::null::<u64>().read_volatile();
        }
    }
}

