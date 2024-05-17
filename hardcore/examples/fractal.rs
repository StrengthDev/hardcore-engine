use async_trait::async_trait;
use tracing::trace;
use tracing_subscriber::filter::LevelFilter;
use tracing_subscriber::EnvFilter;

use hardcore::event::{Event, WindowEvent};
use hardcore::layer::{Context, Layer};
use hardcore::window::Window;
use hardcore::{init, push_layer, run, stop, terminate, ApplicationDescriptor};

struct FractalLayer {
    _window: Window,
}

impl FractalLayer {
    fn new() -> Self {
        Self {
            _window: Window::new(1920, 1080, None, None, "Hardcore sample - Fractal")
                .expect("Failed to create window"),
        }
    }
}

#[async_trait]
impl Layer for FractalLayer {
    async fn tick(&mut self, _context: &Context) {
        // nothing
    }

    async fn handle_event(&mut self, event: &Event) -> bool {
        trace!("Event: {event:?}");
        if let Event::Window {
            event: WindowEvent::Close,
            ..
        } = event
        {
            stop();
        }
        false
    }
}

fn main() {
    let format = tracing_subscriber::fmt::format()
        .with_target(true)
        .with_source_location(false)
        .with_thread_names(true)
        .compact();
    tracing_subscriber::fmt()
        .with_env_filter(
            EnvFilter::try_from_default_env()
                .unwrap_or(EnvFilter::default().add_directive(LevelFilter::TRACE.into())),
        )
        .event_format(format)
        .init();

    init(ApplicationDescriptor {
        name: "Hardcore Fractal sample",
        major: env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
        minor: env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
        patch: env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
    })
    .expect("Failed to initialise library");
    push_layer(FractalLayer::new()).expect("Failed to send fractal layer");
    run().expect("Failed to run main loop");
    terminate().expect("Failed to terminate library");
}
