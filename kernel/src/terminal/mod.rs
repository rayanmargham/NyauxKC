use core::ptr::{null, null_mut};
use flantermbindings::flanterm::*;
use flantermbindings::flanterm::flanterm_fb_init;
use limine::framebuffer::Framebuffer;
pub static mut FLANTERM_CONTEXT: *mut flanterm_context = null_mut();
// #[repr(C)]
// #[derive(Copy, Clone, NoUninit)]
// struct GST {
//     limit: u16,
//     base: u16,
//     base: u8,

// }

unsafe fn uinit_term(fb: &mut Framebuffer) {
    unsafe {
    FLANTERM_CONTEXT = flanterm_fb_init(
        None,
        None,
        fb.addr().cast(),fb.width() as usize, fb.height() as usize,fb.pitch() as usize,
        fb.red_mask_size(), fb.red_mask_shift(),
        fb.green_mask_size(),fb.green_mask_shift(),
        fb.blue_mask_size(), fb.blue_mask_shift(),
        null_mut(),
        null_mut(),null_mut(),
        null_mut(),null_mut(),
        null_mut(),null_mut(),
        null_mut(), 0, 0, 1,
        0, 0,
        0
    ) };
}
pub fn init_term(fb: &mut Framebuffer) {
    unsafe {
        uinit_term(fb);
    }
}