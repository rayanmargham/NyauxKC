

use crate::arch::Processor;
use crate::arch::Arch;
use crate::arch::risc_v::interrupts::setup_interrupts;
use crate::arch::risc_v::pt::PTENT;
use crate::arch::risc_v::pt::phys_to_virt;
use crate::println;
#[cfg(target_arch = "riscv64")]
pub mod interrupts;
pub mod pt;
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

    }
    
    fn get_root_table() -> *mut u64 {
        let phys = unsafe {
            let x: usize;
            core::arch::asm!("csrr {}, satp", out(reg) x);
            (x & 0xFFF_FFFF_FFFF) << 12
        };
        core::ptr::with_exposed_provenance_mut(phys)
    }
    
    fn pt_init() -> (usize, usize) {
        pt::pt_init()
    }

}

