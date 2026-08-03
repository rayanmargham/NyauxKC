use crate::{
    println, status, util::{EResult, Once, RWSpinLock, SpinLockB, errno}, vfs::ustar::load_ramfs_with_ustar_tar,
};
use alloc::{
    boxed::Box,
    string::String,
    sync::{Arc, Weak},
    vec::Vec,
};
pub mod ramfs;
pub mod ustar;
pub enum v_type {
    VREG,
    VDIR,
    // more when we support more bullshit
}
pub struct vfs {
    v_root: Arc<RWSpinLock<vnode>>,
    v_next: Option<Arc<RWSpinLock<vfs>>>,
    v_ops: Box<dyn vfsops>,
}
trait vfsops: Send + Sync {}
pub type vf = Arc<RWSpinLock<vfs>>;
pub type vv = Arc<RWSpinLock<vnode>>;
pub struct vnode {
    v_type: v_type,
    v_prev_data: Option<Box<dyn vops>>, // for mounts
    v_data: Box<dyn vops>,
    v_vfs: Option<Weak<RWSpinLock<vfs>>>,
}
impl vnode {
    fn new(v_vfs: Option<vf>, v_type: v_type, v_data: Box<dyn vops>) -> Self {
        vnode {
            v_type: v_type,
            v_data: v_data,
            v_prev_data: None,
            v_vfs: v_vfs.and_then(|f| Some(Arc::downgrade(&f.clone()))),
        }
    }
}
pub struct dentry {
    name: String,
    inner: Option<vv>,
}
pub struct vattr {
    vtype: v_type,
}

trait vops: Send + Sync {
    fn read(&self, buf: &mut [u8], offset: usize) -> isize;
    fn write(&mut self, buf: &[u8], offset: usize) -> isize;
    fn create(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv>; // creates a file
    fn mkdir(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv>;
    fn mount(&mut self, v: vv);
    fn unmount(&mut self, v: vv);
    // all dentry stuff is put on the fs level
    // the callee will implement their way of dentry caching
    // based on however the fs is
    // in case you forget rayan its like so
    // dentry is a solution to a problem of "how do we read directories without asking the disk everytime cause thats slow as balls"
    // you cache if say there are entries in the directory or there arent or something like that
    // it can be more complex but for nyaux for now it will be implemented as "does directory have dentries?"
    // if yes return, if no dont do shit, if uncached go check
    fn lookup(&self, str: &str) -> Option<vv>;
}
struct nullfs {}
impl vops for nullfs {
    fn create(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
        return Err(errno::EOPNOTSUPP);
    }
    fn lookup(&self, str: &str) -> Option<vv> {
        None
    }
    fn mkdir(&mut self, vfs: Option<Weak<RWSpinLock<vfs>>>, name: &str) -> EResult<vv> {
        unreachable!()
    }
    fn mount(&mut self, v: vv) {
        unreachable!()
    }
    fn read(&self, buf: &mut [u8], offset: usize) -> isize {
        unreachable!()
    }
    fn write(&mut self, buf: &[u8], offset: usize) -> isize {
        unreachable!()
    }
    fn unmount(&mut self, v: vv) {
        unreachable!()
    }
}
// mounts work like this, say we have 2 vnodes
// a is ramfs, b is ext2
// it works like so
// you can just lookup on a for b
// and be will just have different fs ops because b is a different fs and will have a different vfs
// basically instead of v_vfsmountedhere we just make the root vnode of a mounted vfs also the vnode that the vfs is mounted
fn mount(vnode: vv, mut vnodeops: Box<dyn vops>) -> EResult<()> {
    vnodeops.mount(vnode.clone());
    let mut g = vnode.lock_write();
    if g.v_prev_data.is_some() {
        return Err(errno::EBUSY);
    }
    let o = core::mem::replace(&mut g.v_data, Box::new(nullfs {}));

    g.v_prev_data = Some(o);
    g.v_data = vnodeops;
    Ok(())
}
fn unmount(vnode: vv) -> EResult<()> {
    vnode.lock_write().v_data.unmount(vnode.clone());
    let mut g = vnode.lock_write();

    let o = core::mem::replace(&mut g.v_prev_data, None);
    g.v_data = o.unwrap();
    Ok(())
}

fn lookup(vnod: vv, path: &str) -> Option<vv> {
    // TODO
    let mut a: Arc<RWSpinLock<vnode>> = vnod;
    println!("i got given path {}", path);
    for i in path.split_terminator('/') {
        if i.is_empty() {
            continue;
        }

        println!("{:?} len {}", i, i.len());
        // a is always some intially so this is fine twinaling
        let h = a.lock_read().v_data.lookup(i);

        match h {
            Some(f) => {
                a = f;
            }
            None => {
                return None;
                
            }
        }
    }
    return Some(a);
}

impl vfs {
    fn new(fs: (Box<dyn vfsops>, Box<dyn vops>)) -> vf {
        let v = Arc::new(RWSpinLock::new(vnode::new(None, v_type::VDIR, fs.1)));
        let s = Arc::new(RWSpinLock::new(vfs {
            v_root: v.clone(),
            v_next: None,
            v_ops: fs.0,
        }));
        v.lock_write().v_vfs = Some(Arc::downgrade(&s.clone()));
        s
    }
    fn inner_root_vnode(&self) -> vv {
        self.v_root.clone()
    }
}

static ROOTFS: Once<vf> = Once::new();
pub fn vfs_init() {
    // init with ramfs
    ROOTFS.call_once(|| {
        vfs::new((
            Box::new(ramfs::ramfs {}),
            Box::new(ramfs::ramfsdir::new())
        ))
    });
    load_ramfs_with_ustar_tar();
    status!("ramfs inited!");
}
