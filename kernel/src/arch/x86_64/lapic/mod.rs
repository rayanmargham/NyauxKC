use crate::{arch::x86_64::{CalibrationTimer, cpuid, rdmsr, wrmsr}, get_cpu_local, println, util::to_hhdm};

const IA32_APIC_BASE_MSR: u32 = 0x1B;
const IA32_X2APIC_BASE_MSR: u32 = 0x800;

macro_rules! x2apic_read_reg {
    ($reg:expr) => {
        // cool trick by water bottle
        rdmsr(IA32_X2APIC_BASE_MSR + ($reg >> 4))
    };
}

macro_rules! x2apic_write_reg {
    ($reg:expr, $val:expr) => {
        wrmsr(IA32_X2APIC_BASE_MSR + ($reg >> 4), $val)
    };
}

pub struct lapicdata {
    lapic_addr: Option<*mut u64>,
    lapic_id: u32,
    ticks_per_ms: usize,
    bsp: bool
}
pub enum lapic {
    xapic(lapicdata),
    x2apic(lapicdata)
}
impl lapic {
    fn new() -> Self {
        let mut data = lapicdata {lapic_addr: None, lapic_id: 0, bsp: false, ticks_per_ms: 0};
        if enter_x2apic(&mut data).is_ok() {
            Self::x2apic(data)
        } else {
            Self::xapic(data)
        }
    }
    fn init(&mut self) -> Result<(), &'static str> {
        match self {
            lapic::xapic(lapicdata) => {
                todo!();
            },
            lapic::x2apic(lapicdata) => {
                lapicdata.lapic_id = x2apic_read_reg!(0x20) as u32;
                println!("x2apic id {}",lapicdata.lapic_id);
                // spurious interrupt at 0x20
                x2apic_write_reg!(0xF0, (0xFF) | (1 << 8));
                return Ok(());
            }
        }
        Err("init failed")
    }
    fn calibrate_timer(&mut self, cali: &dyn CalibrationTimer) {
        match self {
            lapic::x2apic(lapicdata) => {
                // 0x380 intial count
                // 0x390 current count
                // 0x3E0 divide configuration register
                // 0x320 lvt timer reg
                // timer on 0x21, masked
                x2apic_write_reg!(0x320, 0x21 | (1 << 16));
                x2apic_write_reg!(0x3E0, 0xb); // divide by 1
                x2apic_write_reg!(0x380, 0xFFFFFFFF);
                cali.poll_for_ms(10);
                let cur = x2apic_read_reg!(0x390);
                let ticks_per_ms = (0xFFFFFFFF - cur) / 10;
                lapicdata.ticks_per_ms = ticks_per_ms;
                println!("ticks per 1ms {}", ticks_per_ms);
            },
            lapic::xapic(lapicdata) => {todo!()}
        }
    }
}
fn enter_x2apic(data: &mut lapicdata) -> Result<(), &'static str>{
    let mut addr = rdmsr(IA32_APIC_BASE_MSR);
    if addr & (1 << 8) != 0 {
        data.bsp = true;
    }
    if !(addr & (1 << 11) != 0) {
        addr |= (1 << 11);
        wrmsr(IA32_APIC_BASE_MSR, addr);
        addr = rdmsr(IA32_APIC_BASE_MSR);
    }
    let features = cpuid(1, 0);
    if !(features.2 & (1 << 21) != 0) {

        data.lapic_addr = Some(to_hhdm((addr & !0xFFF) as *mut u64));
        return Err("nyaux does not support cpus that do not support x2apic due to a few reasons");
    }
    if !(addr & (1 << 10) != 0) {
        addr |= (1 << 10);
        wrmsr(IA32_APIC_BASE_MSR, addr);
        addr = rdmsr(IA32_APIC_BASE_MSR);
    }
    println!("entered x2apic mode");
    Ok(())
}
pub fn lapic_init(cali: &dyn CalibrationTimer) {
    let mut l = lapic::new();
    l.init().unwrap();
    l.calibrate_timer(cali);
    let ll = unsafe {get_cpu_local!().as_mut().unwrap()};
    ll.lapic = Some(l);
}