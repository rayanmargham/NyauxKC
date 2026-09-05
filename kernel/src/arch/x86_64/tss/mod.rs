use crate::{arch::Processor, println};

#[repr(C, packed)]
pub struct tss {
    pub reserved: u32,
    pub rsp0: u64,
    pub rsp1: u64,
    pub rsp2: u64,
    pub reserved_1: u64,
    pub ist1: u64,
    pub ist2: u64,
    pub ist3: u64,
    pub ist4: u64,
    pub ist5: u64,
    pub ist6: u64,
    pub ist7: u64,
    pub reserved_2: u64,
    pub reserved_3: u16,
    pub iopb: u16,
}

impl tss {
    pub const fn new() -> tss {
        tss {
            reserved: 0,
            rsp0: 0,
            rsp1: 0,
            rsp2: 0,
            reserved_1: 0,
            ist1: 0,
            ist2: 0,
            ist3: 0,
            ist4: 0,
            ist5: 0,
            ist6: 0,
            ist7: 0,
            reserved_2: 0,
            reserved_3: 0,
            iopb: size_of::<tss>() as u16,
        }
    }
    pub fn ltss(desc: u16) {
        unsafe { core::arch::asm!("ltr ax", in("ax") desc) };
    }
}
