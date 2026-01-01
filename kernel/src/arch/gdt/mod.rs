use bytemuck::{Pod, Zeroable};


#[repr(C)]
#[derive(Pod, Copy, Clone, Zeroable)]
pub struct GDTdesc {
    limit_low: u16,
    base_low: u16,
    base_mid: u8,
    access_byte: u8,
    flags_and_limhi: u8,
    base_hi: u8
}