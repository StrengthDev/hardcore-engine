use std::num::NonZeroU64;
use tracing::{debug, trace};
use tracing_subscriber::filter::LevelFilter;
use tracing_subscriber::EnvFilter;

use hardcore::context::Context;
use hardcore::event::{Event, WindowEvent};
use hardcore::input::{ButtonAction, MouseButton};
use hardcore::layer::Layer;
use hardcore::render::vulkan_api_version;
use hardcore::resource::VertexBuffer;
use hardcore::shader::Shader;
use hardcore::window::{CursorMode, Window};
use hardcore::{ApplicationDescriptor, Instance, Version};

use hardcore_sys::ShaderStage;

struct FractalLayer<'c> {
    _window: Option<Window<'c>>,
    obj: Option<VertexBuffer<'c, false>>,
    print_signal: bool,
    action_signal: bool,
    vert_shader: Shader,
    frag_shader: Shader,
    cursor_mode: bool,
}

impl<'c> FractalLayer<'c> {
    fn new(context: &mut Context<'c>) -> Self {
        let mut s = Self {
            _window: None,
            obj: None,
            print_signal: false,
            action_signal: false,
            vert_shader: Shader::try_from_source(
                include_str!("resources/shaders/shader.vert"),
                ShaderStage::Vertex.into(),
                Default::default(),
            )
            .expect("Failed to create shaders"),
            frag_shader: Shader::try_from_source(
                include_str!("resources/shaders/shader.frag"),
                ShaderStage::Fragment.into(),
                Default::default(),
            )
            .expect("Failed to create shader"),
            cursor_mode: false,
        };

        // TODO think of how something like a LayerID can be passed in the constructor so window doesnt have to be an optional
        s._window = Some(
            context.devices[0]
                .create_window(&s, 1920, 1080, None, None, "Hardcore sample - Fractal")
                .expect("Failed to create window"),
        );

        s
    }
}

impl<'c> Layer<'c> for FractalLayer<'c> {
    fn tick(&mut self, context: &mut Context<'c>) {
        if self.print_signal {
            self.print_signal = false;
            debug!("Vulkan {}", vulkan_api_version());
            for (i, device) in context.devices.iter().enumerate() {
                debug!("Device {i} name: {}", device.name())
            }
        }

        if self.action_signal {
            if self.obj.is_none() {
                let desc = hardcore::descriptor![float];
                let _ = self.obj.insert(
                    context.devices[0]
                        .create_vertex_buffer(self, &desc, NonZeroU64::new(1000).unwrap())
                        .expect("Failed to allocate vertex buffer"),
                );
            } else {
                self.obj.take();
            }

            self.action_signal = false;
        }

        // if let Some(x) = &self.obj {
        //     x.clone();
        // }
        // nothing
    }

    fn handle_event(&mut self, context: &mut Context<'c>, event: &Event) -> bool {
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
                self.action_signal = true;
            }
            Event::Window {
                event:
                    WindowEvent::MouseButton {
                        button: MouseButton::Button2,
                        action: ButtonAction::Release,
                        ..
                    },
                ..
            } => self.print_signal = true,
            Event::Window {
                event:
                    WindowEvent::MouseButton {
                        button: MouseButton::Button4,
                        action: ButtonAction::Release,
                        ..
                    },
                ..
            } => {
                self.cursor_mode = !self.cursor_mode;
                let cursor_mode = if self.cursor_mode {
                    CursorMode::Disabled
                } else {
                    CursorMode::Normal
                };

                if let Some(window) = &mut self._window {
                    window.set_cursor_mode(cursor_mode)
                }
            }
            Event::Window {
                event: WindowEvent::Close,
                ..
            } => {
                context.exit();
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

    let instance = Instance::create(ApplicationDescriptor {
        name: "Hardcore Fractal sample",
        version: Version {
            major: env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
            minor: env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
            patch: env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
        },
    })
    .expect("Failed to initialise library");
    instance
        .run(move |context| {
            let layer = FractalLayer::new(context);
            context.push_layer(layer);
        })
        .expect("Failed to run main loop");
}
