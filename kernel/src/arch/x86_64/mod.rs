use crate::arch::{Arch, Processor};



pub mod gdt;
pub mod idt;


#[cfg(target_arch = "x86_64")]
impl Arch for Processor{
    fn arch_init() -> Result<(), &'static str> {
        use crate::println;

        gdt::gdt_init();
        idt::idt_init();
        println!("trying out explosion tactic (causing page fault)");
        unsafe {
            let v = 0 as *mut i32;
            *v = 5;
        }
        Ok(())
    }
}

