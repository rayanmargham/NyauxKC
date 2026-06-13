use core::slice::{from_raw_parts, from_raw_parts_mut};

use alloc::vec;

use crate::{
    HHDM_REQUEST,
    memory::pmm::allocate_page,
    pci::{
        PCI_BUS_MASTER, PCI_MEM_SPACE, pci_devices, pci_map_bar, pci_read_byte, pci_read_dword,
        pci_read_word, pci_write_word,
    },
    println, status,
    util::{Once, SpinLockB},
};

const VIRTIO_GPU_DEVICE_ID: u16 = 0x1050;
const VIRTIO_PCI_CAP_COMMON_CFG: u8 = 1;
const VIRTIO_PCI_CAP_NOTIFY_CFG: u8 = 2;
const VIRTIO_PCI_CAP_ISR_CFG: u8 = 3;
const VIRTIO_PCI_CAP_DEVICE_CFG: u8 = 4;
const VIRTIO_PCI_CAP_PCI_CFG: u8 = 5;
const VIRTIO_PCI_CAP_SHARED_MEMORY_CFG: u8 = 8;
const VIRTIO_PCI_CAP_VENDOR_CFG: u8 = 9;
const VIRTIO_ACKNOWLEDGE: usize = 1;
const VIRTIO_DRIVER: usize = 2;
const VIRTIO_FAILED: usize = 128;
const VIRTIO_FEATURES_OK: usize = 8;
const VIRTIO_DRIVER_OK: usize = 4;
const VIRTIO_F_VERSION_1: usize = (1 << 32);
const VIRTIO_DEVICE_NEEDS_RESET: usize = 64;
#[derive(Debug)]
pub struct virtio_pci_cap {
    cfg_type: u8,
    bar: u8,
    id: u8,
    offset: u32,
    length: u32,
    notify_off_multi: Option<u32>,
}
const VIRTQ_DESC_F_NEXT: u16 = 1;
const VIRTQ_DESC_F_WRITE: u16 = 2;
const VIRTQ_DESC_F_INDIRECT: u16 = 4;

const VIRTQ_AVAIL_F_NO_INTERRUPT: u16 = 1;

const QUEUE_SIZE: usize = 256;
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct virtq_desc {
    addr: u64,
    len: u32,
    flags: u16,
    next: u16,
}
impl virtq_desc {
    fn next<'a>(&self, pool: &'a freelistdescpool) -> Option<&'a virtq_desc> {
        (self.flags & VIRTQ_DESC_F_NEXT != 0)
            .then(|| pool.get_desc_reference(self.next))
            .flatten()
    }
}
#[repr(C)]
pub struct virtq_avail {
    flags: u16,
    idx: u16,
    //ring: [u16; QUEUE_SIZE],
    // we do not support VIRTIO_F_EVENT_IDX
}
impl virtq_avail {}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct virtq_used_elem {
    id: u32, // index of descriptor chain
    len: u32,
}
#[repr(C)]
pub struct virtq_used {
    flags: u16,
    idx: u16,
    //ring: [virtq_used_elem; QUEUE_SIZE],
}
struct freelistdescpool {
    virtq_desc_pool: *mut virtq_desc,
    queue_size: usize,
    head: u16,
    num_free: u16,
}
pub struct virtq {
    consumer_used_idx: u16,
    desc: freelistdescpool,
    avail: *mut virtq_avail,
    used: *mut virtq_used,
    notify: Option<*mut u16>,
}

#[repr(C)]
#[derive(Debug)]
struct virtio_gpu_ctrl_hdr {
    typ: u32,
    flags: u32,
    fence_id: u64,
    ctx_id: u32,
    ring_idx: u8,
    padding: [u8; 3],
}
#[repr(C)]
#[derive(Debug)]
struct virtio_gpu_rect {
    x: u32,
    y: u32,
    width: u32,
    height: u32,
}
#[repr(C)]
#[derive(Debug)]
struct virtio_gpu_display_one {
    r: virtio_gpu_rect,
    enabled: u32,
    flags: u32,
}
const VIRTIO_GPU_MAX_SCANOUTS: usize = 16;
const VIRTIO_GPU_CMD_GET_DISPLAY_INFO: u32 = 0x0100;
#[repr(C)]
#[derive(Debug)]
struct virtio_gpu_resp_display_info {
    hdr: virtio_gpu_ctrl_hdr,
    pmodes: [virtio_gpu_display_one; VIRTIO_GPU_MAX_SCANOUTS],
}
impl virtq {
    fn new(queue_size: usize) -> virtq {
        virtq {
            consumer_used_idx: 0,
            desc: freelistdescpool::new(queue_size),
            avail: allocate_page().cast(),
            used: allocate_page().cast(),
            notify: None,
        }
    }
    fn expose_queue_addrs(&self) -> (*mut virtq_desc, *mut virtq_avail, *mut virtq_used) {
        (self.desc.virtq_desc_pool, self.avail, self.used)
    }
    fn avail_ring(&mut self) -> &mut [u16] {
        unsafe {
            let ptr = (self.avail as *mut u8)
                .add(core::mem::size_of::<virtq_avail>())
                .cast::<u16>();

            core::slice::from_raw_parts_mut(ptr, self.desc.queue_size as usize)
        }
    }
    fn notify(&mut self, queue_idx: u16) {
        let notify = self.notify.unwrap();
        unsafe {
            core::ptr::write_volatile(notify, queue_idx);
        }
    }
    // fn submit(&mut self, command_chain: u16) {
    //     let q = self.desc.queue_size;
    //         let a = unsafe {&mut (*self.avail)};
    //         unsafe {
    //             core::ptr::write_volatile(
    //                 core::ptr::addr_of_mut!(self.avail_ring()[a.idx as usize % q]),
    //                 command_chain,
    //             )
    //         };
    //         // make sure command chain is on the ring before we inc idx
    //         core::sync::atomic::fence(core::sync::atomic::Ordering::Release);
    //         unsafe {
    //             core::ptr::write_volatile(
    //                 core::ptr::addr_of_mut!(a.idx),
    //                 a.idx.wrapping_add(1),
    //             );
    //         }
    //     }
    fn used_ring(&mut self) -> &mut [virtq_used_elem] {
        unsafe {
            let ptr = (self.used as *mut u8)
                .add(core::mem::size_of::<virtq_used>())
                .cast::<virtq_used_elem>();

            core::slice::from_raw_parts_mut(ptr, self.desc.queue_size as usize)
        }
    }
    fn read_used_queue_blocking(&mut self) -> virtq_used_elem {
        let mut deviceidx = 0;
        loop {
            deviceidx = unsafe { core::ptr::read_volatile(core::ptr::addr_of!((*self.used).idx)) };
            if deviceidx != self.consumer_used_idx {
                break;
            }
            core::hint::spin_loop();
        }
        core::sync::atomic::fence(core::sync::atomic::Ordering::Acquire);
        let q = self.desc.queue_size;
        let h = self.consumer_used_idx;
        let cal = unsafe {
            core::ptr::read_volatile(core::ptr::addr_of!(self.used_ring()[h as usize % q]))
        };
        self.consumer_used_idx = self.consumer_used_idx.wrapping_add(1);
        return cal;
    }
    fn submit_to_queue(&mut self, command_chain: u16) -> Result<(), ()> {
        let a = unsafe { (&mut *self.avail) };
        let la = a.idx.wrapping_sub(self.consumer_used_idx);
        if la >= self.desc.queue_size as u16 {
            println!("queue full try again later");
            return Err(());
        }
        self.submit(command_chain);
        Ok(())
    }
    fn submit(&mut self, command_chain: u16) {
        let q = self.desc.queue_size;

        let idx = unsafe { core::ptr::read_volatile(core::ptr::addr_of!((*self.avail).idx)) };

        unsafe {
            core::ptr::write_volatile(
                core::ptr::addr_of_mut!(self.avail_ring()[idx as usize % q]),
                command_chain,
            )
        };

        core::sync::atomic::fence(core::sync::atomic::Ordering::Release);

        unsafe {
            core::ptr::write_volatile(
                core::ptr::addr_of_mut!((*self.avail).idx),
                idx.wrapping_add(1),
            );
        }
    }
}
impl freelistdescpool {
    fn get_desc_reference(&self, idx: u16) -> Option<&virtq_desc> {
        let ok = unsafe {
            self.virtq_desc_pool
                .cast::<virtq_desc>()
                .add(idx as usize)
                .as_ref()
        };
        return ok;
    }
    fn get_desc_mutable(&mut self, idx: u16) -> Option<&mut virtq_desc> {
        let ok = unsafe {
            self.virtq_desc_pool
                .cast::<virtq_desc>()
                .add(idx as usize)
                .as_mut()
        };
        return ok;
    }

    fn new(queue_size: usize) -> Self {
        let mut bla = unsafe {
            from_raw_parts_mut(
                allocate_page().cast::<virtq_desc>().as_mut().unwrap(),
                queue_size,
            )
        };
        for i in bla.iter_mut().enumerate() {
            i.1.next = (i.0 as u16) + 1;
        }
        bla[queue_size - 1].next = 0;
        let head = 0;
        Self {
            virtq_desc_pool: bla.as_mut_ptr().cast(),
            head: head,
            num_free: queue_size as u16,
            queue_size: queue_size,
        }
    }
    fn allocate_chain(&mut self, chainlength: u16) -> Result<u16, ()> {
        if chainlength == 0 || self.num_free < chainlength {
            return Err(());
        }
        let t = unsafe {
            from_raw_parts_mut(
                self.virtq_desc_pool.cast::<virtq_desc>().as_mut().unwrap(),
                self.queue_size,
            )
        };
        let head = self.head;
        let mut cur = head;
        for _ in 0..chainlength - 1 {
            t[cur as usize].flags |= VIRTQ_DESC_F_NEXT;
            cur = t[cur as usize].next;
        }
        let o = &mut t[cur as usize];
        self.num_free -= chainlength;
        // o.flags |= VIRTQ_DESC_F_WRITE;
        o.flags &= !VIRTQ_DESC_F_NEXT;
        self.head = o.next;
        o.next = 0;
        return Ok(head);
    }
    fn deallocate_chain(&mut self, headofthechain: u16) -> Result<(), ()> {
        let t = unsafe {
            from_raw_parts_mut(
                self.virtq_desc_pool.cast::<virtq_desc>().as_mut().unwrap(),
                self.queue_size,
            )
        };
        let mut cur = &mut t[headofthechain as usize];
        while cur.flags & VIRTQ_DESC_F_NEXT != 0 {
            cur.flags = 0;
            self.num_free += 1;
            cur = &mut t[cur.next as usize]
        }
        cur.next = self.head;
        cur.flags = 0;
        self.num_free += 1;
        self.head = headofthechain;

        Ok(())
    }
}
#[repr(C)]
struct virtio_pci_common_cfg {
    device_feature_select: u32,
    device_feature: u32,
    driver_feature_select: u32,
    driver_feature: u32,
    config_msix_vector: u16,
    num_queues: u16,
    device_status: u8,
    config_generation: u8,

    queue_select: u16,
    queue_size: u16,
    queue_msix_vector: u16,
    queue_enable: u16,
    queue_notify_off: u16,
    queue_desc: u64,
    queue_driver: u64,
    queue_device: u64,
    queue_notif_config_data: u16,
    queue_reset: u16,

    admin_queue_index: u16,
    admin_queue_num: u16,
}
// ptr, field to read
macro_rules! cm_cfg_read_field {
    ($ptr:expr, $field:ident) => {{ unsafe { core::ptr::read_volatile(core::ptr::addr_of!((*$ptr).$field)) } }};
}
// ptr, field to read, value
macro_rules! cm_cfg_write_field {
    ($ptr:expr, $field:ident, $value:expr) => {{ unsafe { core::ptr::write_volatile(core::ptr::addr_of_mut!((*$ptr).$field), $value) } }};
}
unsafe impl Sync for virtq {}
unsafe impl Send for virtq {}
static virtq: Once<SpinLockB<virtq>> = Once::new();
pub fn init_virtiogpu() {
    let device_list = pci_devices.get().unwrap();
    if let Some(virtio_gpu_location) = device_list.iter().find(|x| {
        let ve = pci_read_word(x.0, x.1, x.2, 0x0);
        let de = pci_read_word(x.0, x.1, x.2, 0x2);
        de == VIRTIO_GPU_DEVICE_ID && ve == 0x1AF4
    }) {
        println!("found virtio gpu device");
        let mut cmd = pci_read_word(
            virtio_gpu_location.0,
            virtio_gpu_location.1,
            virtio_gpu_location.2,
            0x4,
        );
        cmd |= PCI_BUS_MASTER; // so we can do dma
        cmd |= PCI_MEM_SPACE; // mmio
        pci_write_word(
            virtio_gpu_location.0,
            virtio_gpu_location.1,
            virtio_gpu_location.2,
            0x4,
            cmd,
        );
        println!("enabled bus mastering and mmio for virtio gpu");
        let status = pci_read_word(
            virtio_gpu_location.0,
            virtio_gpu_location.1,
            virtio_gpu_location.2,
            0x6,
        );
        if (status & (1 << 4)) == 0 {
            println!("no cap. returning");
            return;
        }
        println!("reading for cap");
        let mut cap = pci_read_byte(
            virtio_gpu_location.0,
            virtio_gpu_location.1,
            virtio_gpu_location.2,
            0x34,
        ) as u16;
        let mut caps = vec![];
        loop {
            let id = pci_read_byte(
                virtio_gpu_location.0,
                virtio_gpu_location.1,
                virtio_gpu_location.2,
                cap,
            );
            let next = pci_read_byte(
                virtio_gpu_location.0,
                virtio_gpu_location.1,
                virtio_gpu_location.2,
                cap + 1,
            );
            if id == VIRTIO_PCI_CAP_VENDOR_CFG {
                let cfg_type = pci_read_byte(
                    virtio_gpu_location.0,
                    virtio_gpu_location.1,
                    virtio_gpu_location.2,
                    cap + 3,
                );
                match cfg_type {
                    VIRTIO_PCI_CAP_COMMON_CFG
                    | VIRTIO_PCI_CAP_DEVICE_CFG
                    | VIRTIO_PCI_CAP_NOTIFY_CFG => {
                        let cap_str = virtio_pci_cap {
                            cfg_type: cfg_type,
                            bar: pci_read_byte(
                                virtio_gpu_location.0,
                                virtio_gpu_location.1,
                                virtio_gpu_location.2,
                                cap + 4,
                            ),
                            id: pci_read_byte(
                                virtio_gpu_location.0,
                                virtio_gpu_location.1,
                                virtio_gpu_location.2,
                                cap + 5,
                            ),
                            offset: pci_read_dword(
                                virtio_gpu_location.0,
                                virtio_gpu_location.1,
                                virtio_gpu_location.2,
                                cap + 8,
                            ),
                            length: pci_read_dword(
                                virtio_gpu_location.0,
                                virtio_gpu_location.1,
                                virtio_gpu_location.2,
                                cap + 12,
                            ),
                            notify_off_multi: {
                                if cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG {
                                    Some(pci_read_dword(
                                        virtio_gpu_location.0,
                                        virtio_gpu_location.1,
                                        virtio_gpu_location.2,
                                        cap + 16,
                                    ))
                                } else {
                                    None
                                }
                            },
                        };
                        println!("found required cap {:#?}", cap_str);
                        caps.push(cap_str);
                    }
                    _ => {}
                }
            }
            if next == 0 {
                break;
            }
            cap = next as u16;
        }
        let cm_ca = caps
            .iter()
            .find(|x| x.cfg_type == VIRTIO_PCI_CAP_COMMON_CFG)
            .unwrap();
        let notif = caps
            .iter()
            .find(|x| x.cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG)
            .unwrap();
        let cm_cfg = pci_map_bar(
            virtio_gpu_location,
            cm_ca.bar as u16,
            cm_ca.offset,
            cm_ca.length as usize,
        )
        .unwrap()
        .cast::<virtio_pci_common_cfg>();
        println!(
            "num of queues virtio gpu supports {}",
            cm_cfg_read_field!(cm_cfg, num_queues)
        );
        assert!(2 <= cm_cfg_read_field!(cm_cfg, num_queues));

        cm_cfg_write_field!(cm_cfg, device_status, 0);
        println!("successfully reset the virtio device");
        cm_cfg_write_field!(
            cm_cfg,
            device_status,
            (VIRTIO_ACKNOWLEDGE | VIRTIO_DRIVER) as u8
        );
        println!("attempting to negotiate with the virtio gpu");
        // use whatever features virtio offers
        cm_cfg_write_field!(cm_cfg, device_feature_select, 0);
        let low = cm_cfg_read_field!(cm_cfg, device_feature) as usize;
        cm_cfg_write_field!(cm_cfg, device_feature_select, 1);
        let hi = cm_cfg_read_field!(cm_cfg, device_feature) as usize;
        println!("virtio gpu supports {:b}", (hi << 32) | low);

        if ((hi << 32) | low) & (VIRTIO_F_VERSION_1) == 0 {
            panic!("nyaux needs virtio version 1");
        }

        cm_cfg_write_field!(cm_cfg, driver_feature_select, 1);
        cm_cfg_write_field!(cm_cfg, driver_feature, 1 as u32);
        cm_cfg_write_field!(cm_cfg, driver_feature_select, 0);
        cm_cfg_write_field!(cm_cfg, driver_feature, 0 as u32);
        let mut st = cm_cfg_read_field!(cm_cfg, device_status);
        cm_cfg_write_field!(cm_cfg, device_status, st | VIRTIO_FEATURES_OK as u8);
        st = cm_cfg_read_field!(cm_cfg, device_status);
        if st & (VIRTIO_FEATURES_OK as u8) != 0 {
            println!("successfully negotiated features with the virtio gpu");
        } else {
            println!("virtio gpu does not support version 1 giving up init");
            return;
        }
        cm_cfg_write_field!(cm_cfg, queue_select, 0);
        let notify_base = unsafe {
            pci_map_bar(
                virtio_gpu_location,
                notif.bar as u16,
                notif.offset,
                notif.length as usize,
            )
            .unwrap()
            .byte_add(
                cm_cfg_read_field!(cm_cfg, queue_notify_off) as usize
                    * notif.notify_off_multi.unwrap() as usize,
            )
        };

        let mut size = cm_cfg_read_field!(cm_cfg, queue_size);
        if size < QUEUE_SIZE as u16 {
            println!("using smaller queue size of {}", size);
        } else {
            size = QUEUE_SIZE as u16;
        }
        cm_cfg_write_field!(cm_cfg, queue_size, size);
        let mut v = virtq::new(size as usize);
        v.notify = Some(notify_base.cast());
        unsafe {
            virtq.call_once(|| SpinLockB::new(v));
        }
        let mut a = virtq.get().unwrap().lock().expose_queue_addrs();
        unsafe {
            a.0 =
                a.0.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize);
            a.1 =
                a.1.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize);
            a.2 =
                a.2.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize);
        }
        cm_cfg_write_field!(cm_cfg, queue_desc, a.0.addr() as u64);
        cm_cfg_write_field!(cm_cfg, queue_driver, a.1.addr() as u64);
        cm_cfg_write_field!(cm_cfg, queue_device, a.2.addr() as u64);
        cm_cfg_write_field!(cm_cfg, queue_enable, 1);
        st = cm_cfg_read_field!(cm_cfg, device_status);
        st |= VIRTIO_DRIVER_OK as u8;
        cm_cfg_write_field!(cm_cfg, device_status, st);
        st = cm_cfg_read_field!(cm_cfg, device_status);
        if st & VIRTIO_FAILED as u8 != 0 {
            panic!("so basically we are kinda fucked");
        }
        if st & VIRTIO_DRIVER_OK as u8 != 0 {
            status!("virtio gpu");
        } else {
            panic!("fuck");
        }
        println!("attempting to get display modes");
        let mut guh = virtq.get().unwrap().lock();
        let head = guh.desc.allocate_chain(2).unwrap();
        let req = allocate_page().cast::<virtio_gpu_ctrl_hdr>();
        let resp = allocate_page().cast::<virtio_gpu_resp_display_info>();
        unsafe {
            *req = virtio_gpu_ctrl_hdr {
                typ: VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
                flags: 0,
                fence_id: 0,
                ctx_id: 0,
                ring_idx: 0,
                padding: [0; 3],
            };
            let desc0 = guh.desc.get_desc_mutable(head).unwrap();
            desc0.addr = req.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize).addr() as u64;
            desc0.len = core::mem::size_of::<virtio_gpu_ctrl_hdr>() as u32;
            desc0.flags = VIRTQ_DESC_F_NEXT;
            desc0.next = head+1;
            let desc1 = guh.desc.get_desc_mutable(head+1).unwrap();
            desc1.addr = resp.byte_sub(HHDM_REQUEST.response().unwrap().offset as usize).addr() as u64;
            desc1.len = core::mem::size_of::<virtio_gpu_resp_display_info>() as u32;
            desc1.flags = VIRTQ_DESC_F_WRITE;
            desc1.next = 0;
        }
    guh.submit_to_queue(head).unwrap();
    guh.notify(0);
    let used = guh.read_used_queue_blocking();
    println!("yooo");
    unsafe {
        println!("gpu said {:?}", (*resp));
    }
    }
}
