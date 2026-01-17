

use crate::arch::Processor;
use crate::arch::Arch;
use crate::println;

#[cfg(target_arch = "riscv64")]
impl Arch for Processor{
    const PAGE_SIZE: usize = 4096;
    fn arch_init(){
        use crate::memory::pmm;

        println!("RISCV64 init");
        pmm::init();
    }
}

