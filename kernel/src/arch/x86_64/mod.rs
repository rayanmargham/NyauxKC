use crate::arch::{Arch, Processor};
#[cfg(target_arch = "x86_64")]
use crate::memory::vmm::Pagemap;



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
    fn arch_map_region(pagemap: Pagemap, base: usize, length: usize, flags: crate::memory::vmm::VMMFlags) {
            use crate::arch::x86_64::pt::PTENT;

            
            let yo = PTENT(pagemap.arch_page);
            for i in (base..length).step_by(Processor::PAGE_SIZE) {
                use crate::{arch::x86_64::pt::PT, memory::pmm::allocate_page};

                yo.map4kib(i as u64, allocate_page().addr() as u64, PT::from_vmmflags(flags)).unwrap();
                
            } 
    }
}