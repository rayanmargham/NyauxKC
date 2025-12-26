// flanterm shit


use core::fmt::Write;
use flantermbindings::*;
pub use flanterm::{flanterm_context, flanterm_write};

use crate::MAXLENGTHOFMSG;
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
            flanterm_write(self.0, s.as_ptr() as *const i8, s.len());
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
        $crate::print!("{}\n", format_args!($($arg)*));
    }};
}