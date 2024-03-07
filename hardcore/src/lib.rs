#![warn(missing_docs)]

use hardcore_sys::InitParams;

pub mod window;

pub fn init() {
    let params = InitParams { log_fn: None };

    let res: i32 = unsafe { hardcore_sys::init(params) };

    if res < 0 {
        panic!("Error initialising")
    }
}

pub fn terminate() {
    let res: i32 = unsafe { hardcore_sys::term() };

    if res < 0 {
        panic!("Error terminating")
    }
}
