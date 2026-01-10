use bytemuck::{Pod, Zeroable};
use core::mem::size_of;
use core::ptr::addr_of;

use crate::println;


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

#[repr(C, align(16))]
pub struct GdtTable {
    entries: [GDTdesc; 3]
}

static GDT_TABLE: GdtTable = GdtTable {
    entries: [
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
            let limhi = 0xF;  // High 4 bits of limit
            let l = 1;
            let db = 0;
            let g = 1;
            (g << 7) | (db << 6) | (l << 5) | limhi
        },
        base_hi: 0x0,
    },
    GDTdesc {
        limit_low: 0xFFFF,
        base_low: 0x0,
        base_mid: 0x0,
        base_hi: 0x0,
        access_byte: {
            let p = 1;
            let dpl = 0;
            let s = 1;
            let e = 0;
            let dc = 0;
            let rw = 1;
            let a = 0;
            (p << 7) | (dpl << 6) | (s << 4) | (e << 3) | (dc << 2) | (rw << 1) | 0
        },
        flags_and_limhi: {
            let limhi = 0xF;  // High 4 bits of limit
            let l = 0;
            let db = 1;
            let g = 1;
            (g << 7) | (db << 6) | (l << 5) | limhi
        },
    }
]};

static mut GDTR: gdtr = gdtr {
    size: 0,
    offset: 0
};

pub fn gdt_init() {
    unsafe {
        GDTR.size = (size_of::<[GDTdesc; 3]>() - 1) as u16;
        GDTR.offset = addr_of!(GDT_TABLE.entries) as u64;
    }

    unsafe {
        core::arch::asm!(
            "lgdt [{}]",
            in(reg) &raw const GDTR,
        );
        core::arch::asm!("
            push 0x1
            lea rax, 2f
            push rax
            retfq
            2:
            mov ax, 0x10")
    }
    
}