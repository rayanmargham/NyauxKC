#![no_std]
#![no_main]
pub mod ft;
pub mod arch;
pub mod memory;

use limine::request::HhdmRequest;
unsafe extern "C" {
    pub static KS: u8;
}
#[used]
#[unsafe(link_section = ".requests")]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();
use core::arch::asm;
use core::ptr::addr_of;
use core::{alloc, u64};

use flantermbindings::flanterm::flanterm_fb_init;
use limine::BaseRevision;
use limine::request::{FramebufferRequest, RequestsEndMarker, RequestsStartMarker};
use crate::arch::{Arch, Processor};
use crate::ft::init_terminal;
use crate::memory::pmm::{self, allocate_page, deallocate_page};
use crate::memory::slab::{slab_alloc, slab_dealloc};
#[inline]
const fn align_up(value: u64, alignment: u64) -> u64 {
    (value + alignment - 1) & !(alignment - 1)
}

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
static BASE_REVISION: BaseRevision = BaseRevision::new();

#[used]
#[unsafe(link_section = ".requests")]
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

/// Define the stand and end markers for Limine requests.
#[used]
#[unsafe(link_section = ".requests_start_marker")]
static _START_MARKER: RequestsStartMarker = RequestsStartMarker::new();
#[used]
#[unsafe(link_section = ".requests_end_marker")]
static _END_MARKER: RequestsEndMarker = RequestsEndMarker::new();

#[unsafe(no_mangle)]
unsafe extern "C" fn kmain() -> ! {
    // All limine requests must also be referenced in a called function, otherwise they may be
    // removed by the linker.
    assert!(BASE_REVISION.is_supported());

    if let Some(framebuffer_response) = FRAMEBUFFER_REQUEST.get_response() {
        if let Some(framebuffer) = framebuffer_response.framebuffers().next() {
            unsafe {
                init_terminal(flanterm_fb_init(None, None, framebuffer.addr() as *mut u32, framebuffer.width() as usize, framebuffer.height() as usize, framebuffer.pitch() as usize, framebuffer.red_mask_size(), framebuffer.red_mask_shift(), framebuffer.green_mask_size(), framebuffer.green_mask_shift(), framebuffer.blue_mask_size(), framebuffer.blue_mask_shift(), 0 as *mut u32, 0 as *mut u32, 0 as *mut u32, 0 as *mut u32, 0 as *mut u32, 0 as *mut u32, 0 as *mut u32, 0 as *mut _, 0, 0, 0, 1, 1, 50));
            }
            Processor::arch_init();
            
            println!(
                "Nyaux on the Nya Kernel!"
            );
            println!("testing slab allocation");
            let bro = slab_alloc(size_of::<[i32; 100]>()).unwrap().cast::<[i32; 100]>();
            unsafe {
            for i in 0..100 {
                bro.add(i).cast::<i32>().write(i as i32);
                assert_eq!(bro.add(i).cast::<i32>().read(), i as i32);
            } }
            slab_dealloc(bro.cast());

            status!("slab");
     

        } 
    }

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
