#![no_std]
#![feature(debug_closure_helpers, ptr_metadata, unsafe_cell_access)]
#![allow(nonstandard_style, unused_variables, unused)]
#![no_main]
pub mod arch;
pub mod bootstrap;
pub mod ft;
pub mod memory;
pub mod pci;
pub mod scheduler;
pub mod uacpi;
pub mod util;
pub mod virtio;
//pub mod vfs;
extern crate alloc;

// GCC runtime helper not provided by compiler_builtins on RISC-V without Zbb.
// ffs(v): index of lowest set bit (1-indexed), or 0 if v == 0.
#[cfg(target_arch = "riscv64")]
#[unsafe(no_mangle)]
extern "C" fn __ffsdi2(v: u64) -> i32 {
    if v == 0 {
        0
    } else {
        v.trailing_zeros() as i32 + 1
    }
}
use limine::request::{ExecutableAddressRequest, HhdmRequest, RsdpRequest};
unsafe extern "C" {
    pub static KS: u8;
}
#[used]
#[unsafe(link_section = ".requests")]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();
#[used]
#[unsafe(link_section = ".requets")]
static RSDP_REQUEST: RsdpRequest = RsdpRequest::new();
use core::arch::asm;
use core::ptr::{addr_of, addr_of_mut, null_mut};

#[cfg(target_arch = "x86_64")]
use crate::arch::x86_64::is_intel;
use crate::arch::{Arch, Processor, cpu_local};
use crate::bootstrap::bootstrap_cpus;
use crate::ft::init_terminal;
use crate::memory::ALLOCATER;
use crate::memory::pmm::{self, allocate_page, deallocate_page};
use crate::memory::slab::{slab_alloc, slab_dealloc};
use crate::memory::vmm::{self, VMMFlags, kermap};
use crate::scheduler::sched_init;
use crate::uacpi::init_uacpi;
use crate::virtio::init_virtiogpu;
use flantermbindings::flanterm::flanterm_fb_init;
use limine::BaseRevision;
use limine::request::FramebufferRequest;
#[inline]
const fn align_up(value: u64, alignment: u64) -> u64 {
    (value + alignment - 1) & !(alignment - 1)
}
#[used]
#[unsafe(link_section = ".requests")]
static KERNELADDR_REQUEST: ExecutableAddressRequest = ExecutableAddressRequest::new();
#[inline]
const fn align_down(value: u64, alignment: u64) -> u64 {
    value & !(alignment - 1)
}
/// Sets the base revision to the latest revision supported by the crate.
/// See specification for further info.
/// Be sure to mark all limine requests with #[used], otherwise they may be removed by the compiler.
#[used]
// The .requests section allows limine to find the requests faster and more safely.
#[unsafe(link_section = ".requests")]
static BASE_REVISION: BaseRevision = BaseRevision::with_revision(5);

#[used]
#[unsafe(link_section = ".requests")]
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

#[used]
#[unsafe(link_section = ".requests")]
static FLANTERM_PARAMS: limine::request::FlantermParamsRequest =
    limine::request::FlantermParamsRequest::new();

unsafe extern "C" fn ffree(ptr: *mut ::core::ffi::c_void, size: usize) {
    unsafe {ALLOCATER.dealloc(ptr.cast(), size)};
}
unsafe extern "C" fn fmalloc(size: usize) -> *mut ::core::ffi::c_void {
    unsafe {ALLOCATER.alloc(size) as *mut _}
}
#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    // All limine requests must also be referenced in a called function, otherwise they may be
    // removed by the linker.

    assert!(BASE_REVISION.is_supported());
    pmm::init();
    Processor::arch_bsp_init();
    vmm::vmm_init();
    if let Some(framebuffer_response) = FRAMEBUFFER_REQUEST.response() {
        let framebuffer = framebuffer_response.framebuffers().iter().next().unwrap();
        let entries = FLANTERM_PARAMS.response().unwrap().entries();
        let param = entries[0];
        unsafe {init_terminal(flanterm_fb_init(
            Some(fmalloc),
            Some(ffree),
            framebuffer.address() as _,
            framebuffer.width as _,
            framebuffer.height as _,
            framebuffer.pitch as _,
            framebuffer.red_mask_size as _,
            framebuffer.red_mask_shift as _,
            framebuffer.green_mask_size as _,
            framebuffer.green_mask_shift as _,
            framebuffer.blue_mask_size as _,
            framebuffer.blue_mask_shift as _,
            {if let Some(c) = param.canvas() {
                c.as_ptr() as *mut _
            }else {
                null_mut()
            }},
            param.ansi_colors.as_ptr() as *mut _,
            param.ansi_bright_colors.as_ptr() as *mut _,
            &raw const param.default_bg as *mut _,
            &raw const param.default_fg as *mut _,
            &raw const param.default_bg_bright as *mut _,
            &raw const param.default_fg_bright as *mut _,
            param.font().unwrap().font.as_ptr() as *mut _,
            param.font().unwrap().width as _,
            param.font().unwrap().height as _,
            param.font().unwrap().spacing as _,
            param.font().unwrap().scale_x as _,
            param.font().unwrap().scale_y as _,
            param.margin as _,
        ));}
    }


    println!("Nyaux on the Nya Kernel!");
    println!("testing slab allocation");
    let bro = slab_alloc(size_of::<[i32; 100]>())
        .unwrap()
        .cast::<[i32; 100]>();
    unsafe {
        for i in 0..100 {
            bro.cast::<i32>().add(i).write(i as i32);
            assert_eq!(bro.cast::<i32>().add(i).read(), i as i32);
        }
    }
    slab_dealloc(bro.cast());

    status!("slab");
    let map = unsafe { (&mut *addr_of_mut!(kermap)).as_mut().unwrap() };
    let yaho = unsafe {
        (&mut *core::ptr::addr_of_mut!(kermap))
            .as_mut()
            .unwrap()
            .vmm_alloc(Processor::PAGE_SIZE * 10, VMMFlags::WRITE)
            .unwrap()
    };
    let yaho = yaho.cast::<u8>();
    println!("vmm_alloc returned: 0x{:x}", yaho.addr());
    unsafe {
        for i in 0..(Processor::PAGE_SIZE * 10) {
            yaho.add(i).write(0xAB);
        }
        for i in 0..(Processor::PAGE_SIZE * 10) {
            assert_eq!(yaho.add(i).read(), 0xAB);
        }
    }
    status!("vmm_alloc works");
    map.vmm_dealloc(yaho.cast(), false);
    init_uacpi();
    #[cfg(target_arch = "x86_64")]
    if is_intel() {
        use crate::arch::x86_64::intel::iommu::iommu_init;

        iommu_init();
    }
    cpu_local::new(true);
    Processor::init_timer();
    bootstrap_cpus();

    sched_init();

    hcf();
}

fn kentry() {
    println!("hello from kernel thread scheduled by scheduler");
    println!("wassup twin");
    init_virtiogpu();
    hcf();
}
#[cfg(not(test))]
#[panic_handler]
fn rust_panic(info: &core::panic::PanicInfo) -> ! {
    println!("{}: {}", info.location().unwrap(), info.message());
    loop {
        unsafe {
            #[cfg(target_arch = "x86_64")]
            asm!("hlt");
            #[cfg(any(target_arch = "aarch64", target_arch = "riscv64"))]
            asm!("wfi");
            #[cfg(target_arch = "loongarch64")]
            asm!("idle 0");
        }
    }
}
#[inline]
fn hcf() -> ! {
    loop {
        unsafe {
            #[cfg(target_arch = "x86_64")]
            asm!("hlt");
            #[cfg(any(target_arch = "aarch64", target_arch = "riscv64"))]
            asm!("wfi");
            #[cfg(target_arch = "loongarch64")]
            asm!("idle 0");
        }
    }
}
