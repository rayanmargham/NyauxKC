use limine::{mp::MpInfo, request::MpRequest};

use crate::println;

#[used]
#[unsafe(link_section = ".requests")]
pub static multi_proc: MpRequest = MpRequest::new(0);
pub fn bootstrap_cpus() {
    if let Some(res) = multi_proc.response() {
        #[cfg(target_arch = "x86_64")]
        {
            use crate::arch::Processor;

            for i in res.cpus() {
                use crate::arch::x86_64::ap_init;

                i.bootstrap(ap_init, i.extra_argument());
            }
        }
        #[cfg(target_arch = "riscv64")]
        {
            println!("todo so not booting up other risc v cores");
        }
    }
}
