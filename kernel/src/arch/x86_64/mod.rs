use core::arch::naked_asm;
#[cfg(target_arch = "x86_64")]
use core::mem::offset_of;

#[cfg(target_arch = "x86_64")]
use alloc::{boxed::Box, sync::Arc};
use limine::mp::MpInfo;

use crate::{
    arch::{
        Arch, Processor,
        x86_64::{gdt::ap_gdt_init, idt::idt_load},
    },
};
#[cfg(target_arch = "x86_64")]
use crate::{
    arch::{
        cpu_local, x86_64::{gdt::GdtTable, idt::x86_64Context, intel::iommu::iommu_init, lapic::lapic_init, pt::pt_init},
    }, memory::vmm::Pagemap, println, scheduler::thread,
};

pub mod gdt;
pub mod hpet;
pub mod idt;
pub mod intel;
pub mod lapic;
pub mod pt;
pub mod serial;
pub mod tss;
// pub mod abi;

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
        );
    }
}
// clanker functions cpuid and is_intel produced by clanker because
// i need to do the iommu right now and i cannot be fucked to
// worry about this right now, looks correct to me anyway
/// returns (eax, ebx, ecx, edx)
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
    unsafe { core::arch::asm!("rdmsr", in("ecx") msr, out("eax") lo, out("edx") hi) };
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
impl Arch for Processor {
    type CPUContext = x86_64Context;
    fn Prepare_thread(slice: &mut [usize], entry: usize, stack_ptr: *mut (), user: bool) {
        if user {
            todo!()
        }
        let meow = bytemuck::try_from_bytes_mut::<x86_64Context>(bytemuck::try_cast_slice_mut(slice).unwrap()).unwrap();
        meow.rip = entry as u64;
        meow.rsp = stack_ptr.expose_provenance() as u64;
        meow.cs = offset_of!(GdtTable, kernelcode) as u64;
        meow.ss = offset_of!(GdtTable, kerneldata) as u64;
        
    }
    fn context_switch<T: super::ArchContext>(old_thread: Option<Arc<thread>>, new_thread: &[usize], frame: &mut T) {
        let fram = unsafe {
            (frame as *mut T as *mut x86_64Context).as_mut().unwrap()
        };
        let fra = bytemuck::try_cast_slice_mut::<u8, usize>(bytemuck::bytes_of_mut(fram)).unwrap();
        if let Some(old_thr) = old_thread {
            let bypass = unsafe{(Arc::as_ptr(&old_thr) as *mut crate::scheduler::thread).as_mut().unwrap().cpucontext.as_mut_slice()};
            bypass.copy_from_slice(fra);
        }

        fra.copy_from_slice(new_thread);
        
    }
    fn is_interrupts_enabled() -> bool {
        let mut rflags: usize = 0;
        unsafe {
            core::arch::asm!(
                "pushfq
                pop {x}",
                x = out(reg) rflags
            );
        }
        if rflags & (1 << 9) != 0 {
            true
        } else {
            false
        }
    }

    const PAGE_SIZE: usize = 4096;
    fn arch_bsp_init() {
        use crate::{
            memory::{pmm, vmm},
            println,
        };
        gdt::bsp_gdt_init();
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
                unsafe { core::arch::asm!("in al, dx", out("al") h, in("dx") addr as u16) };
                return h as u64;
            }
            2 => {
                let h: u16;
                unsafe { core::arch::asm!("in ax, dx", out("ax") h, in("dx") addr as u16) };
                return h as u64;
            }
            4 => {
                let h: u32;
                unsafe { core::arch::asm!("in eax, dx", out("eax") h, in("dx") addr as u16) };
                return h as u64;
            }
            _ => {
                panic!("invalid")
            }
        }
    }
    fn raw_io_out(addr: u64, data: u64, byte_width: u8) {
        match byte_width {
            1 => {
                unsafe { core::arch::asm!("out dx, al", in("dx") addr, in("al") data as u8) };
            }
            2 => {
                unsafe { core::arch::asm!("out dx, ax", in("dx") addr, in("ax") data as u16) };
            }
            4 => {
                unsafe { core::arch::asm!("out dx, eax", in("dx") addr, in("eax") data as u32) };
            }
            _ => {}
        }
    }

    fn init_timer() {
        calibrate_timer_init();
        lapic_init(cali_timer.get().unwrap().as_ref());
    }

    fn init_cpu_local(ptr: *mut cpu_local) {
        unsafe {
            wrmsr(GS_BASE, ptr.expose_provenance());
        }
    }
    /// it will set the timer ms then renable the timer
    fn set_timer_ms(ms: usize) {
        use crate::get_cpu_local;
        let local = unsafe { get_cpu_local!().as_mut().unwrap() };
        let l = local.lapic.as_mut().unwrap();
        l.set_timer(ms);
    }
    fn mask_timer() {
        use crate::get_cpu_local;
        let local = unsafe { get_cpu_local!().as_mut().unwrap() };
        let l = local.lapic.as_mut().unwrap();
        l.disable_timer();
    }
    fn acknowledge_interrupt() {
        use crate::get_cpu_local;
        let local = unsafe { get_cpu_local!().as_mut().unwrap() };
        let l = local.lapic.as_mut().unwrap();
        l.send_eoi();
    }
    fn enable_interrupts() {
        unsafe {
            core::arch::asm!("sti");
        }
    }
    fn disable_interrupts() {
        unsafe {
            core::arch::asm!("cli");
        }
    }
    fn set_interrupt_stack(stack_ptr: *mut ()) -> Result<(), &'static str> {
        use crate::get_cpu_local;
        let l = get_cpu_local!();
        let add = stack_ptr.addr();
        let t = unsafe { (*l).tss_ptr };
        unsafe {
            (*t).ist1 = add as u64;
        }
        Ok(())
    }
    fn arch_bootstrap(res: &limine::request::Response<limine::mp::MpRespData>) {
        for i in res.cpus() {
            if i.lapic_id == res.bsp_lapic_id {
                continue;
            }
            println!("booting cpu {}", i.processor_id);
            i.bootstrap(ap_init, 0);
        }
    }
}
pub unsafe extern "C" fn ap_init(info: &MpInfo) -> ! {
    let g = cpu_local::new(false);

    let gtab = unsafe { (*g).gdt.as_ref().unwrap() };
    ap_gdt_init(gtab);
    idt_load();

    status!("cpu {}", info.processor_id);
    loop {
        core::hint::spin_loop();
    }
    unreachable!()
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
#[macro_export]
macro_rules! can_prempt {
    () => {{
        let x: u8;
        unsafe {core::arch::asm!("mov {}, gs:[{off}]", out(reg_byte) x, off = const core::mem::offset_of!(crate::arch::cpu_local, preempt))};
        x != 0
    }};
}
