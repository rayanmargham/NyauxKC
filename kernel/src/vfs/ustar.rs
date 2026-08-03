use core::ffi::CStr;

use bytemuck::{Pod, Zeroable};

use crate::{
    MODULE_REQUEST, align_down, align_up, cmd, print, println, status, vfs::{ROOTFS, lookup, vv},
};

#[repr(C)]
#[derive(Pod, Zeroable, Clone, Copy)]
struct ustar_inode {
    name: [u8; 100],
    file_mode: [u8; 8],
    ownr_user_id: [u8; 8],
    grp_user_id: [u8; 8],
    file_size: [u8; 12],
    mod_time: [u8; 12],
    checksum: [u8; 8],
    typ: [u8; 1],
    link_file_name: [u8; 100],
    ustar_ind: [u8; 6],
    ustar_ver: [u8; 2],
    own_usr_name: [u8; 32],
    own_grp_name: [u8; 32],
    dev_major_num: [u8; 8],
    dev_min_num: [u8; 8],
    filename_prefix: [u8; 155],
    padding: [u8; 12]
}

impl ustar_inode {}
macro_rules! modules {
    () => {{ MODULE_REQUEST.response() }};
}
macro_rules! quick_str_from_ustar {
    ($x:expr) => {{ CStr::from_bytes_until_nul(&$x).unwrap().to_str().unwrap() }};
}
macro_rules! octal_to_num {
    ($x:expr) => {{ usize::from_str_radix(quick_str_from_ustar!($x), 8).unwrap() }};
}
pub fn load_ramfs_with_ustar_tar() {
    if modules!().is_none() {
        panic!(
            "Nyaux Requires an initramfs. Please check the README.MD for how the intiramfs should be given to Nyaux."
        )
    }

    let possibletar = {
        if cmd!().is_empty() {
            // Assume the first module is the USTAR initramfs tar thingy
            let modu = modules!().unwrap();
            modu.modules()[0]
        } else {
            todo!()
        }
    };
    println!("found {}", possibletar.path());
    if !possibletar.path().contains("tar") {
        panic!("Nyaux Requires an initramfs. Not a valid tar was provided");
    }
    let d = possibletar.data();
    let mut ptr = d;
    let mut c = 0;
    let r = &ROOTFS.get().unwrap().lock_read();
    while c != d.len() {

        let fz = transmute_to_ramfs(ptr, r.inner_root_vnode());
        let oo = align_up(fz as u64, 512) as usize;
        let inc = oo + size_of::<ustar_inode>();

        ptr = &ptr[inc..(d.len() - c) as usize];
        if &ptr[..1024].iter().sum::<u8>() == &0 { // end of USTAR is dedicated by two blocks of 512 size of zeros, just directly skip this
            break;
        }
        c += inc;

    }
    status!("ustar has been parsed");

}

const ustar_file: u8 = 48;
const ustar_hardlink: u8 = 49;
const ustar_symlink: u8 = 50;
const ustar_chardev: u8 = 51;
const ustar_blkdev: u8 = 52;
const ustar_dir: u8 = 53;
const named_pipe: u8 = 54;
fn transmute_to_ramfs(ptr: &[u8], fs: vv) -> usize{

    let h = bytemuck::try_from_bytes::<ustar_inode>(&ptr[..size_of::<ustar_inode>()]).unwrap();
    let us = quick_str_from_ustar!(h.ustar_ind);

    let w = quick_str_from_ustar!(h.name);
    println!("ustar ind: {}", us);
    if us != "ustar" || w.is_empty(){
        panic!("invalid tar header");
    }


    let fz = octal_to_num!(h.file_size);
    let mut lastcomp = w.split_terminator('/').last().unwrap();
    let mut cur_node = fs.clone();
    for i in w.split_terminator('/') {
        if i.is_empty() {
            continue;
        }
        let meow = cur_node.lock_read();
        if let Some(h) = meow.v_data.lookup(i) {
            drop(meow);
            cur_node = h;
        } else {
            if i == lastcomp {
                match h.typ[0] {
                    ustar_file => {
                        let pp = meow.v_vfs.clone();
                        println!("creating file");
                        drop(meow);
                        let mut c = cur_node.lock_write();
                        c.v_data.create(pp, i).unwrap();
                        println!("{}", fz);
                        if fz == 0 {
                            panic!("?"); // idk if ustar can have empty files but... we'll see yk! JARONA
                        }
                        c.v_data.write(&ptr[..fz], 0);
                        
                    }
                    ustar_dir => {
                        let pp = meow.v_vfs.clone();

                        drop(meow);
                        cur_node.lock_write().v_data.mkdir(pp, i).unwrap();
                    }
                    _ => {
                        println!("ignoring type of {}", h.typ[0]);
                    }
                }
            } else {
                let pp = meow.v_vfs.clone();

                drop(meow);
                let new = cur_node.lock_write().v_data.mkdir(pp, i).unwrap();
                cur_node = new;
            }
        }
    }
    fz
}
