#![warn(missing_docs)]

use hardcore_sys::{destroy_window, new_window};
use std::ptr::addr_of_mut;

pub struct Window {
    handle: hardcore_sys::Window,
}

impl Window {
    pub fn new() -> Self {
        Self {
            handle: unsafe { new_window() },
        }
    }
}

impl Drop for Window {
    fn drop(&mut self) {
        let ptr = addr_of_mut!(self.handle);
        unsafe {
            destroy_window(ptr);
        }
    }
}
