use alloc::{boxed::Box, string::String, sync::{Arc, Weak}, vec::Vec};

use crate::{util::{EResult, RWSpinLock, errno}, vfs::{dentry, v_type, vfs, vfsops, vnode, vops, vv}};

pub struct ramfs {
// todo
}
impl vfsops for ramfs {

}
pub struct ramfsdir {
    entries: Vec<dentry>,
}
pub struct ramfsfile {
    buf: Vec<u8>
}
impl ramfsdir {
    pub fn new() -> Self {
        ramfsdir { entries: Vec::new() }
    }
    pub fn create_entry(&mut self, name: &str, vn: vv) {
        self.entries.push(dentry {
            name: String::from(name),
            inner: Some(vn)
        });
    }
    
}
impl ramfsfile {
    fn new() -> Self {
        ramfsfile {
            buf: Vec::new()
        }
    }
}
impl vops for ramfsfile {
    fn mount(&mut self, v: vv) {
        
    }
    fn mkdir(&mut self, vnode: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
        return Err(errno::ENOTDIR);
    }
    fn unmount(&mut self, v: vv) {
        
    }
fn create(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
    todo!()
}
    fn lookup(&self, str: &str) -> Option<vv> {
        return None;
    }
    fn read(&self, buf: &mut [u8], offset: usize) -> isize {
        if offset >= self.buf.len() {
            return 0;
        }
        let byte_amount = (buf.len()).min(self.buf.len() - offset);
        let g = &self.buf[offset..offset + byte_amount];
        buf[..byte_amount].copy_from_slice(g);
        return byte_amount as isize;
        todo!()
    }
    fn write(&mut self, buf: &[u8], offset: usize) -> isize {
        // offset = 5
        // self.buf.len = 6
        // buf.len = 2
        if offset + buf.len() > self.buf.len() {
            self.buf.resize(offset + buf.len(), 0);
        }

        self.buf[offset..offset + buf.len()].copy_from_slice(buf);
        return buf.len() as isize;
    }
}
impl vops for ramfsdir {
    fn mount(&mut self, v: vv) {
        
    }
    fn unmount(&mut self, v: vv) {
        
    }
    fn mkdir(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
        let k = Box::new(ramfsdir::new());

        let h = Arc::new(
            RWSpinLock::new(
                vnode::new(
                    vfs.and_then(|f|f.upgrade()).clone(),
                    v_type::VDIR,
                    k
                )
            )
        );
        self.create_entry(name, h.clone());
        Ok(h)
    }
    fn create(&mut self, mut vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
        if self.lookup(name).is_some() {
            return Err(errno::EEXIST);
        }
        
        let newguy = Box::new(ramfsfile::new());
        let n = Arc::new(RWSpinLock::new(vnode::new(vfs.and_then(|f|f.upgrade()).clone(),  v_type::VREG, newguy)));
        self.create_entry(name, n.clone());
        Ok(n)

    }
    fn read(&self, buf: &mut [u8], offset: usize) -> isize{
        // if buf.len() < size_of::<dentry>() {
        //     return 0;
        // }
        // let a = unsafe {core::slice::from_raw_parts(self.entries.as_ptr() as *const u8, self.entries.len() * size_of::<dentry>())};
        // // so you treat entries as a buffer. allowing you to read in the middle of a dentry or at the end or whatever unix wants from us
        // // we do offset+buf.len() because buf is a slice and the len is the number of bytes we want to write into the buffer, caller gives us this slice
        // // with that intention
        // // .min is that so if say buffer was like 100 bytes and we only had 60 bytes in our entries buffer we pick 60 so we dont overread
        // // the - offset is that so if we have an offset of say 5 and our buffer is like 6 we dont overread because we skipped those bytes
        
        // let amount_to_read = buf.len().min(a.len() - offset);
        // let entries_to_read = &a[offset..offset + amount_to_read];
        // buf.copy_from_slice(entries_to_read);
        // // if the buffer is bigger then the array then we only read the 
        // return amount_to_read as isize;
        return -1;
    }
    fn write(&mut self, buf: &[u8], offset: usize) -> isize {
        return -1 ;
    }
    fn lookup(&self, str: &str) -> Option<vv> {
        self.entries
            .iter().find(|a|a.name == str).and_then(|f|f.inner.clone())
    }
}