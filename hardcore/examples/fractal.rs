use async_trait::async_trait;
use std::num::NonZeroU64;
use tracing::{debug, trace};
use tracing_subscriber::filter::LevelFilter;
use tracing_subscriber::EnvFilter;

use hardcore::event::{Event, WindowEvent};
use hardcore::input::{ButtonAction, MouseButton};
use hardcore::layer::{Context, Layer};
use hardcore::resource::VertexBuffer;
use hardcore::window::Window;
use hardcore::{init, push_layer, run, stop, terminate, ApplicationDescriptor};

struct FractalLayer {
    _window: Window,
    obj: Option<VertexBuffer<false>>,
    flag: bool,
}

impl FractalLayer {
    fn new() -> Self {
        Self {
            _window: Window::new(1920, 1080, None, None, "Hardcore sample - Fractal")
                .expect("Failed to create window"),
            obj: None,
            flag: false,
        }
    }
}

#[async_trait]
impl Layer for FractalLayer {
    async fn tick(&mut self, context: &Context) {
        if self.flag {
            self.flag = false;
            for (i, device) in context.devices.iter().enumerate() {
                debug!("Device {i} name: {}", device.name())
            }
        }

        // if let Some(x) = &self.obj {
        //     x.clone();
        // }
        // nothing
    }

    async fn handle_event(&mut self, event: &Event) -> bool {
        trace!("Event: {event:?}");
        match event {
            Event::Window {
                event:
                    WindowEvent::MouseButton {
                        button: MouseButton::Button1,
                        action: ButtonAction::Release,
                        ..
                    },
                ..
            } => {
                if self.obj.is_none() {
                    let desc = hardcore::descriptor![float];
                    let _ = self.obj.insert(
                        VertexBuffer::<false>::create(&desc, NonZeroU64::new(1000).unwrap(), 0)
                            .expect("Failed to allocate vertex buffer"),
                    );
                } else {
                    self.obj.take();
                }
            }
            Event::Window {
                event:
                    WindowEvent::MouseButton {
                        button: MouseButton::Button2,
                        action: ButtonAction::Release,
                        ..
                    },
                ..
            } => self.flag = true,
            Event::Window {
                event: WindowEvent::Close,
                ..
            } => {
                stop();
            }
            _ => { /* nothing */ }
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
