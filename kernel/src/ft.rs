// flanterm shit


use core::fmt::Write;
use flantermbindings::*;
pub use flanterm::{flanterm_context, flanterm_write};

pub struct Ball(*mut flanterm_context);

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

pub static mut TERMINAL: Option<Ball> = None;

pub unsafe fn init_terminal(ctx: *mut flanterm_context) {
    TERMINAL = Some(Ball(ctx));
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        use core::fmt::Write;
        unsafe {
            if let Some(ref mut term) = $crate::ft::TERMINAL {
                let _ = write!(term, $($arg)*);
            }
        }
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

        $crate::print!("{}: {} \x1b[32mOK\x1b[0m\n", module_path!(), format_args!($($arg)*));
    }};
}