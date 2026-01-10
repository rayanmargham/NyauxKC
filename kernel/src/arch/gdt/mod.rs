use bytemuck::{Pod, Zeroable};
use core::mem::{offset_of, size_of};
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
    base_hi: u8,
}

#[repr(C, packed)]
#[derive(Pod, Copy, Clone, Zeroable)]
pub struct gdtr {
    size: u16,
    offset: u64,
}
impl GDTdesc {
    const fn new(p: bool, rw: bool, dc: bool, e: bool, s: bool, dpl: u8) -> Self {
        GDTdesc {
            access_byte: {
                (p as u8) << 7
                    | ((rw as u8) << 1)
                    | ((dc as u8) << 2)
                    | ((e as u8) << 3)
                    | ((s as u8) << 4)
                    | (dpl << 6)
                    | 1
            },
            limit_low: 0xFFFF,
            base_low: 0x0,
            base_mid: 0x0,
            flags_and_limhi: {
                let limhi = 0xF; // High 4 bits of limit
                let l = 1;
                let db = 0;
                let g = 1;
                (g << 7) | (db << 6) | (l << 5) | limhi
            },
            base_hi: 0x0,
        }
    }
}
#[repr(C, align(16))]
pub struct GdtTable {
    null: GDTdesc,
    kernelcode: GDTdesc,
    kerneldata: GDTdesc,
}

static GDT_TABLE: GdtTable = GdtTable {
    null: GDTdesc {
        limit_low: 0x0,
        base_hi: 0x0,
        base_low: 0x0,
        base_mid: 0x0,
        access_byte: 0x0,
        flags_and_limhi: 0x0,
    },
    kernelcode: GDTdesc::new(true, true, false, true, true, 0),
    kerneldata: GDTdesc::new(true, true, false, false, true, 0),
};

pub fn gdt_init() {
    let gdtrr: gdtr = gdtr {
        size: (size_of::<[GDTdesc; 3]>() - 1) as u16,
        offset: addr_of!(GDT_TABLE) as u64,
    };

    unsafe {
        core::arch::asm!(
            "lgdt [{}]",
            in(reg) &gdtrr,
        );
        core::arch::asm!("
            push {kernelcode}
            lea rax, 2f
            push rax
            retfq
            2:
            mov ax, {kerneldata}
            mov ds, ax
            mov es, ax
            mov fs, ax
            mov gs, ax
            mov ss, ax", kernelcode = const offset_of!(GdtTable, kernelcode), kerneldata = const offset_of!(GdtTable, kerneldata))
    }
}
