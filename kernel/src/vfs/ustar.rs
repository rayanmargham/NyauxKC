use crate::{MODULE_REQUEST, cmd, print, println};


#[repr(C)]
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
    filename_prefix: [u8; 155]

}

impl ustar_inode {

}
macro_rules! modules {
    () => {
        {
            MODULE_REQUEST.response()
        }   
    };
}
pub fn load_ramfs_with_ustar_tar() {
    if modules!().is_none() {
        panic!("Nyaux Requires an initramfs. Please check the README.MD for how the intiramfs should be given to Nyaux.")
    }
    if cmd!().is_empty() {
        // Assume the first module is the USTAR initramfs tar thingy
        
    }
}
