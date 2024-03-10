//! TODO

#![warn(missing_docs)]

use hardcore_sys::InitParams;

mod native;
pub mod window;

pub fn init() {
    let params = InitParams {
        log_fn: Some(native::log),
        start_span_fn: Some(native::start_span),
        end_span_fn: Some(native::end_span),
    };

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
