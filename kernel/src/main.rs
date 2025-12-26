#![no_std]
#![no_main]

use core::arch::asm;

use flantermbindings::flanterm::flanterm_fb_init;
use limine::BaseRevision;
use limine::request::{FramebufferRequest, RequestsEndMarker, RequestsStartMarker};
use limine_rust_template::ft::init_terminal;
use limine_rust_template::println;

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
            for i in 0..100_u64 {
                println!("hi sigmas {}", i);
            }
        }
    }

    hcf();
}

#[panic_handler]
fn rust_panic(_info: &core::panic::PanicInfo) -> ! {
    hcf();
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
