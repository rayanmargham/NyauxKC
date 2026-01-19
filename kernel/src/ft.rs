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
fn clean_function_name(full_name: &'static str) -> &'static str {
    // Find the last occurrence of "::" and take everything before it
    let mut who = full_name;
    if let Some(pos) = full_name.rfind("::") {
        who = &who[pos..];
    };
    who

}
fn get_function_name<F>(_: F) -> &'static str 
where
    F: Fn(),
{
    let full = core::any::type_name::<F>();
    clean_function_name(full)
}
        $crate::print!("{}->{}: {}\n", module_path!(), get_function_name(|| {}), format_args!($($arg)*));
    }};
}