use std::thread::sleep;
use std::time::Duration;

use hardcore::window::Window;
use hardcore::{init, terminate};

fn main() {
    init();
    {
        let _window = Window::new();
        sleep(Duration::from_secs(1));
    }
    terminate();
}
