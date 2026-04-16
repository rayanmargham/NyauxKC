use bytemuck::{Pod, Zeroable};
use core::cell::UnsafeCell;
use core::mem::{offset_of, size_of};
use core::ptr::addr_of;

use crate::arch::x86_64::tss::tss;
use crate::println;
use crate::util::SyncCell;

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
                    | (dpl << 5)
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
#[repr(C, packed)]
pub struct GDTtss {
    seg_lim: u16,
    base_addr_low: u16,
    base_addr_low_hi: u8,
    access_byte: u8,
    flags_and_lim: u8,
    base_addr_hi_low: u8,
    base_addr_hi_hi: u32,
    reserved: u32
}
impl GDTtss {
    const fn zeroed() -> GDTtss {
        GDTtss {
            seg_lim: 0,
            base_addr_low: 0,
            base_addr_low_hi: 0,
            access_byte: 0,
            flags_and_lim: 0,
            base_addr_hi_low: 0,
            base_addr_hi_hi: 0,
            reserved: 0,
        }
    }
    pub fn new(tss: *mut tss) -> GDTtss {
        let limit = size_of::<tss>() - 1;
        let addr = tss as usize;
        GDTtss {
            seg_lim: (limit & 0xFFFF) as u16,
            base_addr_low: (addr & 0xFFFF) as u16,
            base_addr_low_hi: ((addr >> 16) & 0xFF) as u8,
            base_addr_hi_low: ((addr >> 24) & 0xFF) as u8,
            base_addr_hi_hi: ((addr >> 32) & 0xFFFFFFFF) as u32,
            access_byte: {
                let typ = 0x9;
                (1 as u8) << 7 |
                typ
            },
            flags_and_lim: {
                ((limit >> 16) & 0xF) as u8
            },
            reserved: 0
        }
    }
}
#[repr(C, align(16))]
pub struct GdtTable {
    null: GDTdesc,
    pub kernelcode: GDTdesc,
    pub kerneldata: GDTdesc,
    pub tss: GDTtss
}
impl GdtTable {
    pub const fn new() -> GdtTable {
        GdtTable {
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
    tss: GDTtss::zeroed()
}
    }
}
pub static BSP_TSS: SyncCell<tss> = SyncCell::new(tss::new());
static BSP_GDT_TABLE: SyncCell<GdtTable> = SyncCell::new(GdtTable::new());

pub fn bsp_gdt_init() {
    BSP_GDT_TABLE.get_mut_unchecked().tss = GDTtss::new(BSP_TSS.get());
    let gdtrr: gdtr = gdtr {
        size: (size_of::<GdtTable>() - 1) as u16,
        offset: addr_of!(BSP_GDT_TABLE) as u64,
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
            mov ss, ax", kernelcode = const offset_of!(GdtTable, kernelcode), kerneldata = const offset_of!(GdtTable, kerneldata), out("rax") _)
    }
    tss::ltss(offset_of!(GdtTable, tss) as u16);
}
pub fn ap_gdt_init(table: &GdtTable) {
    let gdtrr: gdtr = gdtr {
        size: (size_of::<GdtTable>() - 1) as u16,
        offset: table as *const GdtTable as u64,
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
            mov ss, ax", kernelcode = const offset_of!(GdtTable, kernelcode), kerneldata = const offset_of!(GdtTable, kerneldata), out("rax") _)
    }
    tss::ltss(offset_of!(GdtTable, tss) as u16);
}