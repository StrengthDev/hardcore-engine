use std::thread::sleep;
use std::time::Duration;
use tracing_subscriber::filter::LevelFilter;
use tracing_subscriber::EnvFilter;

use hardcore::window::Window;
use hardcore::{init, terminate};

fn main() {
    let format = tracing_subscriber::fmt::format()
        .with_target(true)
        .with_source_location(false)
        .with_thread_names(true)
        .compact();
    tracing_subscriber::fmt()
        .with_env_filter(
            EnvFilter::try_from_default_env()
                .unwrap_or(EnvFilter::default().add_directive(LevelFilter::INFO.into())),
        )
        .event_format(format)
        .init();

    init();
    {
        let _window = Window::new();
        sleep(Duration::from_secs(1));
    }
    terminate();
}
