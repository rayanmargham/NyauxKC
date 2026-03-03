use core::ffi::CStr;

use alloc::{boxed::Box, ffi::CString};
use nyaux_uacpi_bindings::{UACPI_LOG_DEBUG, UACPI_LOG_ERROR, UACPI_LOG_INFO, UACPI_LOG_TRACE, UACPI_LOG_WARN, UACPI_STATUS_OK, UACPI_STATUS_UNIMPLEMENTED, uacpi_bool, uacpi_char, uacpi_cpu_flags, uacpi_firmware_request, uacpi_handle, uacpi_init_level, uacpi_initialize, uacpi_interrupt_handler, uacpi_io_addr, uacpi_log_level, uacpi_pci_address, uacpi_phys_addr, uacpi_size, uacpi_status, uacpi_status_to_string, uacpi_thread_id, uacpi_u8, uacpi_u16, uacpi_u32, uacpi_u64, uacpi_work_handler, uacpi_work_type};

use crate::{HHDM_REQUEST, RSDP_REQUEST, memory::slab::{slab_alloc, slab_dealloc}, print, println, util::SpinLock};
#[unsafe(no_mangle)]
unsafe extern "C" fn uacpi_kernel_alloc(size: uacpi_size) -> *mut core::ffi::c_void {
    if size <= 1024 {
        return slab_alloc(size).unwrap().cast()
    } else {
        panic!("shit");
    }
}
#[unsafe(no_mangle)]
pub unsafe fn uacpi_kernel_free(mem: *mut ::core::ffi::c_void, size_hint: uacpi_size) {
    if size_hint <= 1024 {
        slab_dealloc(mem.cast());
    } else {
        panic!("shit");
    }
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_get_rsdp(out_rsdp_address: *mut uacpi_phys_addr) -> uacpi_status {

    unsafe {out_rsdp_address.write(
        RSDP_REQUEST.response().unwrap().address as u64 - HHDM_REQUEST.response().unwrap().offset
    )};
    UACPI_STATUS_OK
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_map(addr: uacpi_phys_addr, len: uacpi_size) -> *mut ::core::ffi::c_void {
    return core::ptr::with_exposed_provenance_mut::<core::ffi::c_void>(addr as usize+ HHDM_REQUEST.response().unwrap().offset as usize)
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_unmap(addr: *mut ::core::ffi::c_void, len: uacpi_size) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_log(arg1: uacpi_log_level, arg2: *const uacpi_char) {
    match arg1 {
        UACPI_LOG_DEBUG => {
            print!("(debug) {}", CStr::from_ptr(arg2.cast()).to_str().unwrap())

        },
        UACPI_LOG_ERROR => {
            print!("(ERROR) {}", CStr::from_ptr(arg2.cast()).to_str().unwrap())

        },
        UACPI_LOG_INFO => {
            print!("(info) {}", CStr::from_ptr(arg2.cast()).to_str().unwrap())

        },
        UACPI_LOG_TRACE => {
            print!("(trace) {}", CStr::from_ptr(arg2.cast()).to_str().unwrap())

        },
        UACPI_LOG_WARN => {
            print!("(warning) {}", CStr::from_ptr(arg2.cast()).to_str().unwrap())
        },
        _ => {}
    }
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_device_open(
        address: uacpi_pci_address,
        out_handle: *mut uacpi_handle,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
    #[unsafe(no_mangle)]
unsafe extern "C" fn uacpi_kernel_pci_device_close(arg1: uacpi_handle) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_read8(
        device: uacpi_handle,
        offset: uacpi_size,
        value: *mut uacpi_u8,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_read16(
        device: uacpi_handle,
        offset: uacpi_size,
        value: *mut uacpi_u16,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_read32(
        device: uacpi_handle,
        offset: uacpi_size,
        value: *mut uacpi_u32,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_write8(
        device: uacpi_handle,
        offset: uacpi_size,
        value: uacpi_u8,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_write16(
        device: uacpi_handle,
        offset: uacpi_size,
        value: uacpi_u16,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_pci_write32(
        device: uacpi_handle,
        offset: uacpi_size,
        value: uacpi_u32,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_map(
        base: uacpi_io_addr,
        len: uacpi_size,
        out_handle: *mut uacpi_handle,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_unmap(handle: uacpi_handle) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_read8(
        arg1: uacpi_handle,
        offset: uacpi_size,
        out_value: *mut uacpi_u8,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_read16(
        arg1: uacpi_handle,
        offset: uacpi_size,
        out_value: *mut uacpi_u16,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_read32(
        arg1: uacpi_handle,
        offset: uacpi_size,
        out_value: *mut uacpi_u32,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_write8(
        arg1: uacpi_handle,
        offset: uacpi_size,
        in_value: uacpi_u8,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_write16(
        arg1: uacpi_handle,
        offset: uacpi_size,
        in_value: uacpi_u16,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_io_write32(
        arg1: uacpi_handle,
        offset: uacpi_size,
        in_value: uacpi_u32,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_get_nanoseconds_since_boot() -> uacpi_u64 {
0
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_stall(usec: uacpi_u8) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_sleep(msec: uacpi_u64) {

}
#[unsafe(no_mangle)]

unsafe extern "C"  fn uacpi_kernel_create_mutex() -> uacpi_handle {    
    let b = Box::new(0);
    Box::into_raw(b).cast()
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_free_mutex(arg1: uacpi_handle) {
    drop(unsafe {Box::from_raw(arg1)});
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_create_event() -> uacpi_handle {

let b = Box::new(0);
    Box::into_raw(b).cast()
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_free_event(arg1: uacpi_handle) {
    drop(unsafe {Box::from_raw(arg1)});

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_get_thread_id() -> uacpi_thread_id {
0 as uacpi_thread_id
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_acquire_mutex(arg1: uacpi_handle, arg2: uacpi_u16) -> uacpi_status {
        UACPI_STATUS_OK

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_release_mutex(arg1: uacpi_handle) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_wait_for_event(arg1: uacpi_handle, arg2: uacpi_u16) -> uacpi_bool {
    true
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_signal_event(arg1: uacpi_handle) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_reset_event(arg1: uacpi_handle) {

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_handle_firmware_request(arg1: *mut uacpi_firmware_request) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_install_interrupt_handler(
        irq: uacpi_u32,
        arg1: uacpi_interrupt_handler,
        ctx: uacpi_handle,
        out_irq_handle: *mut uacpi_handle,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED

    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_uninstall_interrupt_handler(
        arg1: uacpi_interrupt_handler,
        irq_handle: uacpi_handle,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED
    }
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_create_spinlock() -> uacpi_handle {
    let s = Box::new(SpinLock::new());
    return Box::into_raw(s).cast();
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_free_spinlock(arg1: uacpi_handle) {
    let bro = unsafe {Box::from_raw(arg1)};
    drop(bro);
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_lock_spinlock(arg1: uacpi_handle) -> uacpi_cpu_flags {
    let ar: &SpinLock = unsafe {
        &*arg1.cast::<SpinLock>()
    };
    ar.lock();
    return 0;
}
#[unsafe(no_mangle)]

unsafe extern "C" fn uacpi_kernel_unlock_spinlock(arg1: uacpi_handle, arg2: uacpi_cpu_flags) {
    let ar: &SpinLock = unsafe {
        &*arg1.cast::<SpinLock>()
    };
    ar.unlock();
    
}
#[unsafe(no_mangle)]
unsafe extern "C" fn uacpi_kernel_schedule_work(
        arg1: uacpi_work_type,
        arg2: uacpi_work_handler,
        ctx: uacpi_handle,
    ) -> uacpi_status {
        UACPI_STATUS_UNIMPLEMENTED
    }
    #[unsafe(no_mangle)]
unsafe extern "C" fn uacpi_kernel_wait_for_work_completion() -> uacpi_status {
    UACPI_STATUS_UNIMPLEMENTED
}
pub fn init_uacpi() {
    let mut status = unsafe {uacpi_initialize(0)};
    let m = unsafe {CStr::from_ptr(uacpi_status_to_string(status)).to_str()};
    println!("{}", m.unwrap());
    assert_eq!(status, 0);
}