use core::arch::naked_asm;
#[cfg(target_arch = "x86_64")]
use core::mem::offset_of;

#[cfg(target_arch = "x86_64")]
use alloc::boxed::Box;

use crate::{arch::{Arch, Processor}, scheduler::sched_tramp2};
#[cfg(target_arch = "x86_64")]
use crate::{arch::{cpu_local, x86_64::{intel::iommu::iommu_init, pt::pt_init}}, memory::vmm::Pagemap};



pub mod gdt;
pub mod idt;
pub mod serial;
pub mod pt;
pub mod intel;
pub mod hpet;

pub trait CalibrationTimer {
    fn get_ms(&self) -> usize;
    fn get_ns(&self) -> usize;
    fn poll_for_ms(&self, ms: usize);
}

use crate::{status, util::Once};

pub static cali_timer: Once<Box<dyn CalibrationTimer + Send + Sync>> = Once::new();
pub fn calibrate_timer_init() {
    let hpe = hpet::hpet_init();
    if hpe.is_err() {
        panic!("no hpet no dice sorry");
    }
    cali_timer.call_once(|| Box::new(hpe.unwrap()));
    status!("setup calibration timer");
}
const GS_BASE: u32 = 0xC0000101;
pub fn outb(port: u16, data: u8) {
    unsafe {
    core::arch::asm!(
        "out dx, al",
        in("dx") port,
        in("al") data,
     ); }
    }
// clanker functions cpuid and is_intel produced by clanker because
// i need to do the iommu right now and i cannot be fucked to
// worry about this right now, looks correct to me anyway
pub fn cpuid(leaf: u32, subleaf: u32) -> (u32, u32, u32, u32) {
    let (eax, ebx, ecx, edx): (u32, u32, u32, u32);
    unsafe {
        core::arch::asm!(
            "push rbx",
            "cpuid",
            "mov {ebx_out:e}, ebx",
            "pop rbx",
            inout("eax") leaf => eax,
            inout("ecx") subleaf => ecx,
            ebx_out = out(reg) ebx,
            out("edx") edx,
        );
    }
    (eax, ebx, ecx, edx)
}
pub fn is_intel() -> bool {
    let (_, ebx, ecx, edx) = cpuid(0, 0);
    // "GenuineIntel" = ebx "Genu", edx "ineI", ecx "ntel"
    ebx == 0x756e6547 && edx == 0x49656e69 && ecx == 0x6c65746e
}
pub fn rdmsr(msr: u32) -> usize {
    let mut lo: usize = 0;
    let mut hi: usize = 0;
    unsafe {core::arch::asm!("rdmsr", in("ecx") msr, out("eax") lo, out("edx") hi)};
    return lo | (hi << 32);
}
pub fn wrmsr(msr: u32, val: usize) {
    let hi = (val >> 32);
    let lo = val;
    unsafe {
        core::arch::asm!("wrmsr", in("ecx") msr, in("edx") hi, in("eax") lo);
    }
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
    
    fn init_timer() {
        calibrate_timer_init();
    }
    fn prepare_new_thread_stack(stack_ptr: &mut [usize], function: Box<dyn FnOnce() + 'static + Send>) -> usize {
        let len = stack_ptr.len();
        let slice = &mut stack_ptr[len - 9..];
        slice.fill(0);
        let real = Box::into_raw(function);
        let meta: *const () =  unsafe {core::mem::transmute(core::ptr::metadata(real))};

        slice[8] = meta as usize;
        slice[7] = real.expose_provenance();
        slice[6] = sched_tramp1 as *const fn() as usize;
        9 * size_of::<usize>()

    }
    fn init_cpu_local(ptr: *mut cpu_local) {
        unsafe {
            wrmsr(GS_BASE, ptr.expose_provenance());
        }
    }

}
#[unsafe(naked)]
pub unsafe extern "C" fn sched_tramp1() {
    naked_asm!(
        "mov rax, rdi", // ask emma about this later
        "pop rsi",
        "pop rdx",
        "jmp {}", sym sched_tramp2
    )
}
#[unsafe(naked)]
pub unsafe extern "C" fn context_switch<T>(
    passthrough: *mut (),
    old_stack: *mut *mut (),
    new_stack: *const *mut ()
) -> *const T{
    naked_asm!("
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rsi], rsp
    mov rsp, [rdx]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret");
}
#[macro_export]
macro_rules! get_cpu_local {
    () => {{
        use crate::arch::cpu_local;
        unsafe {
            let x: *mut cpu_local;
            core::arch::asm!("mov {}, gs:[0]", out(reg) x);
            x
        }
    }};
}

