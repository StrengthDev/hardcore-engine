#![warn(missing_docs)]

use hardcore_sys::InitParams;

mod window;

fn init() {
    let params = InitParams { log_fn: None };

    let res: i32 = unsafe { hardcore_sys::init(params) };

    if res < 0 {
        panic!("Error initialising")
    }
}

fn terminate() {
    let res: i32 = unsafe { hardcore_sys::term() };

    if res < 0 {
        panic!("Error terminating")
    }
}

pub fn add(left: usize, right: usize) -> usize {
    left + right
}
#[cfg(test)]
mod tests {
    use super::*;
    use std::thread::sleep;
    use std::time::Duration;
    use window::Window;

    #[test]
    fn it_works() {
        init();
        let _window = Window::new();
        sleep(Duration::from_secs(2));
        let result = add(2, 2);
        assert_eq!(result, 4);
        terminate();
    }
}
