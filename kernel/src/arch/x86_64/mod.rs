use crate::arch::{Arch, Processor};



pub mod gdt;
pub mod idt;
pub mod serial;
pub mod pt;

pub fn outb(port: u16, data: u8) {
    unsafe {
    core::arch::asm!(
        "out dx, al",
        in("dx") port,
        in("al") data,
     ); }
    }
#[cfg(target_arch = "x86_64")]
impl Arch for Processor{
    const PAGE_SIZE: usize = 4096;
    fn arch_init() {
        use crate::{memory::pmm, println};
        println!("x86_64 init");
        gdt::gdt_init();
        idt::idt_init();
        pmm::init();
        pt::pt_init();
        
    }
}