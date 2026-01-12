use core::{arch::naked_asm, mem::offset_of, ptr::addr_of};

use bytemuck::{Pod, Zeroable};
use seq_macro::seq;

use crate::{arch::x86_64::gdt::GdtTable, println};
const INTERRUPT_GATE: u8 = 0xE;
// a lot of code here looks similar to menix, it is true that i took "inspiration" :trollface:, i make sure i understood what im writing
// so i think its fine to have my code REALLY similar to menix. at least these stubs and the singular interrupt handler concept
// as marvin is my bestie westie i will provide his license (obv would even if we weren't friends just joking around)
// Copyright (C) 2025 Marvin Friedrich

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#[repr(C)]
pub struct CPUContext {
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rsi: u64,
    pub rdi: u64,
    pub rbp: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64,
    pub int: u64,
    pub error: u64,
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

#[derive(Pod, Zeroable, Clone, Copy)]
#[repr(C,packed)]
pub struct IDTR {
    size: u16,
    offset: u64
}

#[derive(Pod, Zeroable, Clone, Copy)]
#[repr(C)]
pub struct GateDesc {
    offset_lo: u16,
    seg_sel: u16,
    ist: u8,
    flags: u8,
    offset_mid: u16,
    offset_hi: u32,
    reversed: u32
}
impl GateDesc {
    const fn new(offset: u64, seg: u16, dpl: u8, gate_type: u8, ist: u8) -> Self {
        return GateDesc {
            offset_lo: ((offset as u16) & 0xFFFF),
            seg_sel: seg,
            flags: (1 << 7) | ((dpl & 0x3) << 6) | gate_type & 0xF,
            ist: ist,
            offset_mid: (((offset>> 16) as u16) & 0xFFFF),
            offset_hi: (((offset >> 32) as u32) & 0xFFFFFFFF),
            reversed: 0
        }
    }
}

seq! {N in 0..256 {

#[unsafe(naked)]
unsafe extern "C" fn inter_stub~N() {
    naked_asm!(
        ".if ({i} == 8 || ({i} >= 10 && {i} <= 14) || {i} == 17 || {i} == 21 || {i} == 29 || {i} == 30)
        .else
        push 0
        .endif
        push {i}
        push rax
        push rbx
        push rcx
        push rdx
        push rbp
        push rdi
        push rsi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        mov rdi, rsp
        call {handler}
        jmp {inter_return}
        ",
        i = const N,
        handler = sym idt_handler,
        inter_return = sym inter_return
    );
}}}
unsafe extern "C" fn idt_handler(frame: *mut CPUContext) {
    let int = unsafe {
        frame.as_ref().unwrap().int
    };
    match int {
        _ => {
            panic!("unhandled exception");
        }
    }
}
#[unsafe(naked)]
unsafe extern "C" fn inter_return() {
    naked_asm!("
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rsi
        pop rdi
        pop rbp
        pop rdx
        pop rcx
        pop rbx
        pop rax
        add rsp, 0x16
        iretq");
}
#[derive(Pod, Zeroable, Clone, Copy)]
#[repr(C)]
pub struct IDT {
    entries: [GateDesc; 256]
}

static mut IDT: IDT = IDT {
    entries: [GateDesc::new(0,0,0,0,0); 256]
};

pub fn idt_init() {

        unsafe {
        seq!(N in 0..256 {
            IDT.entries[N] = GateDesc::new((inter_stub~N as *const ()) as u64, (offset_of!(GdtTable, kernelcode) as u16) << 3,0,INTERRUPT_GATE, 0);});};
            let s = addr_of!(IDT);
        let idtr: IDTR = IDTR {
    size: size_of_val(&s) as u16,
    offset: &raw const IDT as u64
};
unsafe {
    core::arch::asm!("
    lidt [{}]", in(reg) &idtr);
}
println!("done? we need to try out an interrupt now");

}