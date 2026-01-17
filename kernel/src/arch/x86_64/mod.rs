use crate::arch::{Arch, Processor};



pub mod gdt;
pub mod idt;


#[cfg(target_arch = "x86_64")]
impl Arch for Processor{
    const PAGE_SIZE: usize = 4096;
    fn arch_init() {
        use crate::{memory::pmm, println};
        println!("x86_64 init");
        gdt::gdt_init();
        idt::idt_init();
        pmm::init();
        
    }
}