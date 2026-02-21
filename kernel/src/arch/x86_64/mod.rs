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
        use crate::{memory::{pmm, vmm}, println};
        println!("x86_64 init");
        gdt::gdt_init();
        idt::idt_init();
        pmm::init();
        vmm::vmm_init();
    }
    fn get_root_table() -> *mut u64 {
        use crate::arch::x86_64::pt::read_cr3;

        core::ptr::with_exposed_provenance_mut::<u64>((read_cr3() as usize & !0xFFF) & !(1 << 63))
    }
}