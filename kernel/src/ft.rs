// flanterm shit


use core::fmt::Write;
use flantermbindings::*;
pub use flanterm::{flanterm_context, flanterm_write};
use crate::util::{Once, SpinLock};

#[derive(Clone, Copy)]
pub struct Ball(*mut flanterm_context);
unsafe impl Send for Ball {}
unsafe impl Sync for Ball {}

impl Write for Ball {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        if self.0.is_null() {
            return Err(core::fmt::Error);
        }

        if s.is_empty() {
            return Ok(());
        }

        unsafe {
            flanterm_write(self.0, s.as_ptr() as *const core::ffi::c_char, s.len());
            #[cfg(target_arch = "x86_64")]
            for i in s.chars() {
                use crate::arch::x86_64::serial::serial_putc;

                serial_putc(i);
            }
        }
        Ok(())
    }
}

pub static TERMINAL: Once<(Ball, SpinLock)> = Once::new();

pub unsafe fn init_terminal(ctx: *mut flanterm_context) {
    TERMINAL.call_once(||(Ball(ctx), SpinLock::new()));
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        use core::fmt::Write;
        $crate::ft::TERMINAL.get().unwrap().1.lock();
        unsafe {
            let mut t = $crate::ft::TERMINAL.get().unwrap().0;
                let _ = write!(t, $($arg)*);
        }
        $crate::ft::TERMINAL.get().unwrap().1.unlock();
    }};
}




#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => {{

        $crate::print!("{}: {}\n", module_path!(), format_args!($($arg)*));
    }};
}
// most useless function
#[macro_export]
macro_rules! status {
    () => {
        
    };
    ($($arg:tt)*) => {{

        $crate::print!("{}: {} \x1b[0m\x1b[32mOK\x1b[0m\n", module_path!(), format_args!($($arg)*));
    }};
}