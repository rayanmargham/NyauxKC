use crate::arch::{Arch, Processor};
#[cfg(target_arch = "x86_64")]
use crate::{arch::x86_64::pt::pt_init, memory::vmm::Pagemap};



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
    }
    fn get_root_table() -> *mut u64 {
        use crate::arch::x86_64::pt::read_cr3;

        core::ptr::with_exposed_provenance_mut::<u64>((read_cr3() as usize & !0xFFF) & !(1 << 63))
    }
    fn pt_init() -> (usize, usize) {
        pt_init()
    }
    fn raw_io_in(addr: u64, byte_width: u8) -> u64 {
        match byte_width {
            1 => {
                let h: u8;
                unsafe {
                core::arch::asm!("in al, dx", out("al") h, in("dx") addr as u16)};
                return h as u64;
            },
            2 => {
                let h: u16;
                unsafe {
                core::arch::asm!("in ax, dx", out("ax") h, in("dx") addr as u16)};
                return h as u64;

            },
            4 => {
                let h: u32;
                unsafe {
                core::arch::asm!("in eax, dx", out("eax") h, in("dx") addr as u16)};
                return h as u64;

            },
            _ => {panic!("invalid")}
        }
    }
    fn raw_io_out(addr: u64, data: u64, byte_width: u8) {
        match byte_width {
            1 => {
                unsafe {
                core::arch::asm!("out dx, al", in("dx") addr, in("al") data as u8)};
            },
            2 => {
                 unsafe {
                core::arch::asm!("out dx, ax", in("dx") addr, in("ax") data as u16)};
            },
            4 => {
                unsafe {
                core::arch::asm!("out dx, eax", in("dx") addr, in("eax") data as u32)};
            },
            _ => {

            }
        }
    }
}