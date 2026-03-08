

use crate::arch::Processor;
use crate::arch::Arch;
use crate::arch::risc_v::interrupts::setup_interrupts;
use crate::arch::risc_v::pt::PTENT;
use crate::arch::risc_v::pt::phys_to_virt;
use crate::println;
use crate::HHDM_REQUEST;
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
    fn raw_io_in(addr: u64, byte_width: u8) -> u64 {
        let addr = addr + HHDM_REQUEST.response().unwrap().offset;
        match byte_width {
            1 => {
                return unsafe {core::ptr::with_exposed_provenance_mut::<u8>(addr as usize).read_volatile()} as u64;
            },
            2 => {
                return unsafe {core::ptr::with_exposed_provenance_mut::<u16>(addr as usize).read_volatile()} as u64;

            },
            4 => {
            return unsafe {core::ptr::with_exposed_provenance_mut::<u32>(addr as usize).read_volatile()} as u64;

            },
            _ => {panic!("wtf")},
        }
    }
    fn raw_io_out(addr: u64, data: u64, byte_width: u8) {
        let addr = addr + HHDM_REQUEST.response().unwrap().offset;
        match byte_width {
            1 => {
                unsafe {core::ptr::with_exposed_provenance_mut::<u8>(addr as usize).write_volatile(data as u8)};
            },
            2 => {
                unsafe {core::ptr::with_exposed_provenance_mut::<u16>(addr as usize).write_volatile(data as u16)};
            },
            4 => {
                unsafe {core::ptr::with_exposed_provenance_mut::<u32>(addr as usize).write_volatile(data as u32)};
            },
            _ => {panic!("wtf")},
        }
    }
}

