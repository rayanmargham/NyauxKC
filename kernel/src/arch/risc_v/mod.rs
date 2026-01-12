

use crate::arch::Processor;
use crate::arch::Arch;
use crate::println;

#[cfg(target_arch = "riscv64")]
impl Arch for Processor{
    fn arch_init() -> Result<(), &'static str> {

        println!("Hello from the Nyaux kernel on risc-v!");
        Ok(())
    }
}

