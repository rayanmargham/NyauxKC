use crate::{hcf, println};

#[repr(C)]
pub struct CPUContext {
    pub t0: u64,
    pub pc: u64,
    pub ra: u64,
    pub sp: u64,
    pub gp: u64,
    pub tp: u64,
    pub t1: u64,
    pub t2: u64,
    pub s0: u64,
    pub s1: u64,
    pub fp: u64,
    pub a0: u64,
    pub a1: u64,
    pub a2: u64,
    pub a3: u64,
    pub a4: u64,
    pub a5: u64,
    pub a6: u64,
    pub a7: u64,
    pub s2: u64,
    pub s3: u64,
    pub s4: u64,
    pub s5: u64,
    pub s6: u64,
    pub s7: u64,
    pub s8: u64,
    pub s9: u64,
    pub s10: u64,
    pub s11: u64,
    pub t3: u64,
    pub t4: u64,
    pub t5: u64,
    pub t6: u64,
}
pub extern "C" fn interrupt_handler(frame: *mut CPUContext) {
    println!("HELLLOOO");
    if let Some(ctx) = unsafe {frame.as_ref()} {
        // figure out if we a exception or what
        let mut scause: u64 = 0;
        unsafe {
            core::arch::asm!("csrr {}, scause", out(reg) scause);
        }
        if scause & (1 << 63) == 0 {
            let exception_code = scause & !(1 << 63);
            let exception_val= {
                let mut hey: u64 = 0;
                unsafe {
                    core::arch::asm!("csrr {}, stval", out(reg) hey);
                }
                hey
            };
            match exception_code {
                13 => {
                    println!("page fault in riscv!!! RIP 0x{:x} faulting address 0x{:x}", ctx.pc, exception_val);
                }
                _ => {}
            }
            hcf();
        }
    }
}
// Save eXecute Return
#[unsafe(naked)]
pub unsafe extern "C" fn context_sxr() {
    // intially wrote the code, clanker fixed the bugs with the risc v inline asm
    // so basically the inline asm is now clanker code. but i understand what its doing
        core::arch::naked_asm!("
            .align 4
            addi sp, sp, -264

            // Save all registers to stack frame (matching CPUContext layout)
            sd t0,    0(sp)
            addi t0, sp, 264
            sd t0,   24(sp)
            csrr t0, sepc
            sd t0,    8(sp)
            sd ra,   16(sp)
            sd gp,   32(sp)
            sd tp,   40(sp)
            sd t1,   48(sp)
            sd t2,   56(sp)
            sd s0,   64(sp)
            sd s1,   72(sp)
            sd fp,   80(sp)
            sd a0,   88(sp)
            sd a1,   96(sp)
            sd a2,  104(sp)
            sd a3,  112(sp)
            sd a4,  120(sp)
            sd a5,  128(sp)
            sd a6,  136(sp)
            sd a7,  144(sp)
            sd s2,  152(sp)
            sd s3,  160(sp)
            sd s4,  168(sp)
            sd s5,  176(sp)
            sd s6,  184(sp)
            sd s7,  192(sp)
            sd s8,  200(sp)
            sd s9,  208(sp)
            sd s10, 216(sp)
            sd s11, 224(sp)
            sd t3,  232(sp)
            sd t4,  240(sp)
            sd t5,  248(sp)
            sd t6,  256(sp)

            mv a0, sp
            call {}

            // Restore sepc
            ld t0,    8(sp)
            csrw sepc, t0

            ld ra,   16(sp)
            // skip sp (24) - restored via addi below
            ld gp,   32(sp)
            ld tp,   40(sp)
            ld t1,   48(sp)
            ld t2,   56(sp)
            ld s0,   64(sp)
            ld s1,   72(sp)
            ld fp,   80(sp)
            ld a0,   88(sp)
            ld a1,   96(sp)
            ld a2,  104(sp)
            ld a3,  112(sp)
            ld a4,  120(sp)
            ld a5,  128(sp)
            ld a6,  136(sp)
            ld a7,  144(sp)
            ld s2,  152(sp)
            ld s3,  160(sp)
            ld s4,  168(sp)
            ld s5,  176(sp)
            ld s6,  184(sp)
            ld s7,  192(sp)
            ld s8,  200(sp)
            ld s9,  208(sp)
            ld s10, 216(sp)
            ld s11, 224(sp)
            ld t3,  232(sp)
            ld t4,  240(sp)
            ld t5,  248(sp)
            ld t6,  256(sp)

            // Restore t0 last (used as scratch above)
            ld t0,    0(sp)

            addi sp, sp, 264
            sret
        ", sym interrupt_handler);
    
}

pub fn setup_interrupts() {
    unsafe {core::arch::asm!("csrw stvec, {}", in(reg) context_sxr)};
}