#![no_std]
pub mod ft;
pub mod arch;
pub mod memory;

use limine::request::HhdmRequest;

#[used]
#[unsafe(link_section = ".requests")]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();