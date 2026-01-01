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


#[repr(C, packed)]
#[derive(Pod, Copy, Clone, Zeroable)]
pub struct gdtr {
    size: u16,
    offset: u64
}

pub static gdt: [GDTdesc; 2] = [
    GDTdesc {
        limit_low: 0x0,
        base_hi: 0x0,
        base_low: 0x0,
        base_mid: 0x0,
        access_byte: 0x0,
        flags_and_limhi: 0x0,
    },
    GDTdesc {
        access_byte: {
            let a: u8 = 0;
            let rw: u8 = 1;
            let dc: u8 = 0;
            let e: u8 = 1;
            let s: u8 = 1;
            let dpl: u8 = 0;
            let p: u8 = 1;
            a | (rw << 1) | (dc << 2) | (e << 3) | (s << 4) | (dpl << 6) | (p << 7) 
        },
        limit_low: 0xFFFF,
        base_low: 0x0,
        base_mid: 0x0,
        flags_and_limhi: {
            let l = 1;
            let db = 0;
            let g = 1;
            g | db | l | 0
        },
        base_hi: 0x0,
    }
]



pub fn gdt_init() {
    let gdtr = gdtr {
        size: (size_of::<[GDTdesc; 2]>() - 1) as u16,
        offset: gdt.as_ptr().addr() as u64
    };


    core::arch::asm!("mov ss, {}")
}