#![cfg(target_arch = "riscv64")]

use core::arch::naked_asm;
use core::ptr::null_mut;

use alloc::boxed::Box;

use crate::arch::Processor;
use crate::arch::Arch;
use crate::arch::risc_v::interrupts::setup_interrupts;
use crate::arch::risc_v::pt::PTENT;
use crate::arch::risc_v::pt::phys_to_virt;
use crate::println;
use crate::HHDM_REQUEST;
use crate::scheduler::sched_tramp2;
use crate::status;
use crate::util::Once;
use crate::util::find_acpi_table;

pub mod interrupts;
pub mod pt;

#[repr(C)]
pub struct sbi_ret {
    error: isize,
    sbi_val: isize // if no negative cast to usize
}
pub fn sbi_call(sbi_extension_id: usize, sbi_function_id: usize, args: [usize; 6]) -> sbi_ret{
    let mut err: isize = 0;
    let mut val: isize = 0;
    unsafe {core::arch::asm!("
    ecall", in("a7") sbi_extension_id, in("a6") sbi_function_id, inout("a0") args[0] as isize => err, inout("a1") args[1] as isize => val, in("a2") args[2], in("a3") args[3], in("a4") args[4], in("a5") args[5])};
    return sbi_ret {error: err, sbi_val: val};
}
#[repr(C, packed)]
pub struct acpi_rhct {
    hdr: nyaux_uacpi_bindings::acpi_sdt_hdr,
    flags: u32,
    tbf: u64,
    num_rhct: u32,
    offset_rhct: u32
}
static timr_freq: Once<u64> = Once::new();
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
    fn init_timer() {
       let tab= find_acpi_table(c"RHCT".as_ptr().cast()).unwrap();
       let rhct_tab = unsafe {tab.__bindgen_anon_1.virt_addr as *mut acpi_rhct};
       let rhct = unsafe {rhct_tab.as_ref().unwrap()};
       let tbf = rhct.tbf;
       println!("RHCT timer freq 0x{:x}", tbf);
       timr_freq.call_once(||tbf);
       let mut sie = unsafe {
        let x: usize;
        core::arch::asm!("csrr {}, sie", out(reg) x);
        x
       };
       sie |= (1 << 5);
       unsafe {
        core::arch::asm!("csrw sie, {}", in(reg) sie);
       }
       let mut sstatus = unsafe {
        let x: usize;
        core::arch::asm!("csrr {}, sstatus", out(reg) x);
        x
       };
       sstatus |= (1 << 1);
       // this is SO stupid even linux does this, risc v people should just
       // die (in minecraft im joking)
       sbi_call(0x54494D45, 0, [usize::MAX, 0, 0, 0, 0, 0]);

       unsafe {
        core::arch::asm!("csrw sstatus, {}", in(reg) sstatus);
       }
       status!("timer");
    }
    fn init_cpu_local(ptr: *mut super::cpu_local) {
        unsafe {
            core::arch::asm!("mv tp, {}", in(reg) ptr.expose_provenance());
        }
    }
    fn prepare_new_thread_stack(stack_ptr: &mut [usize], function: alloc::boxed::Box<dyn FnOnce() + 'static + Send>) -> usize {
        let len = stack_ptr.len();
        let slice = &mut stack_ptr[len - 17..];
        slice.fill(0);
        let real = Box::into_raw(function);
        let meta: *const () =  unsafe {core::mem::transmute(core::ptr::metadata(real))};

        slice[16] = meta as usize;
        slice[15] = real.expose_provenance();
        slice[0] = sched_tramp1 as *const fn() as usize; // todo
        let tp = unsafe {
            let x: usize;
            core::arch::asm!("mv {}, tp", out(reg) x);
            x
        };
        slice[1] = tp;
        17 * size_of::<usize>()
    }
    fn set_timer_ms(ms: usize) {
        todo!();
    }
}
#[unsafe(naked)]
pub unsafe extern "C" fn sched_tramp1() {
    naked_asm!("
    ld a1, 0(sp)
    ld a2, 8(sp)
    addi sp, sp, 16
    j {}", sym sched_tramp2
    )
}
#[unsafe(naked)]
pub unsafe extern "C" fn context_switch<T>(
    passthrough: *mut (),
    old_stack: *mut *mut (),
    new_stack: *const *mut ()
) -> *const T {
    core::arch::naked_asm!("
    addi sp, sp, -120
    sd ra, 0(sp)
    sd tp, 8(sp)
    sd s0, 16(sp)
    sd s1, 24(sp)
    sd s2, 32(sp)
    sd s3, 40(sp)
    sd s4, 48(sp)
    sd s5, 56(sp)
    sd s6, 64(sp)
    sd s7, 72(sp)
    sd s8, 80(sp)
    sd s9, 88(sp)
    sd s10, 96(sp)
    sd s11, 104(sp)
    sd sp, 0(a1)
    ld sp, 0(a2)
    ld s11, 104(sp)
    ld s10, 96(sp)
    ld s9, 88(sp)
    ld s8, 80(sp)
    ld s7, 72(sp)
    ld s6, 64(sp)
    ld s5, 56(sp)
    ld s4, 48(sp)
    ld s3, 40(sp)
    ld s2, 32(sp)
    ld s1, 24(sp)
    ld s0, 16(sp)
    ld tp, 8(sp)
    ld ra, 0(sp)
    addi sp, sp, 120
    ret
    ");
}
#[macro_export]
macro_rules! get_cpu_local {
    () => {{
        use crate::arch::cpu_local;
        unsafe {
            let addr: usize;
            core::arch::asm!("mv {}, tp", out(reg) addr);
            let x = core::ptr::with_exposed_provenance_mut::<cpu_local>(addr);
            x
        }
    }};
}
