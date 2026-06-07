use std::collections::HashMap;
use std::num::NonZeroU32;
use tracing::{debug, trace};
use tracing_subscriber::filter::LevelFilter;
use tracing_subscriber::EnvFilter;

use hardcore::allocator::Allocator;
use hardcore::event::{Event, WindowEvent};
use hardcore::io::input::{ButtonAction, MouseButton};
use hardcore::io::window::{CursorMode, Window};
use hardcore::layer::Layer;
use hardcore::meta::{glfw_version, vulkan_api_version};
use hardcore::ops::raster_commands::Draw;
use hardcore::ops::raster_pipeline::{RasterPipeline, RasterPipelineInfo};
use hardcore::ops::render_pass::{OutputAttachment, RenderPass, SubpassInfo};
use hardcore::resource::TextureDimensions::Texture2D;
use hardcore::resource::{RenderTarget, TextureFormat, TextureView, VertexBuffer};
use hardcore::shader::Shader;
use hardcore::state::State;
use hardcore::{ApplicationDescriptor, Device, Initializer, Instance, Version};
use hardcore_sys::{ShaderStage, TextureComponentFormat, TextureNumericFormat, TextureSampleCount};

struct SharedData<'s> {
    window: Window<'s>,
}

impl<'s> SharedData<'s> {
    fn new(initializer: &'s Initializer, devices: &[Device]) -> Self {
        Self {
            window: initializer
                .new_window(
                    &devices[0],
                    1920,
                    1080,
                    None,
                    None,
                    "Hardcore sample - Fractal",
                )
                .expect("Failed to create window"),
        }
    }
}

struct FractalLayer<'s> {
    obj: Option<VertexBuffer<'s, false>>,
    print_signal: bool,
    action_signal: bool,
    vert_shader: Shader,
    frag_shader: Shader,
    pipeline: RasterPipeline<'s>,
    render_target: RenderTarget<'s>,
    render_pass: RenderPass<'s>,
    draw: Draw<'s>,
    cursor_mode: bool,
    cursor_mode_dirty: bool,
    should_exit: bool,
}

impl<'s> FractalLayer<'s> {
    fn new(initializer: &'s Initializer, devices: &[Device]) -> Self {
        let vert_shader = Shader::try_from_source(
            include_str!("resources/shaders/shader.vert"),
            ShaderStage::Vertex.into(),
            Default::default(),
        )
        .expect("Failed to create vertex shader");
        let frag_shader = Shader::try_from_source(
            include_str!("resources/shaders/shader.frag"),
            ShaderStage::Fragment.into(),
            Default::default(),
        )
        .expect("Failed to create fragment shader");

        let pipeline = initializer
            .new_raster_pipeline(
                &devices[0],
                &[&vert_shader, &frag_shader],
                RasterPipelineInfo::default(),
            )
            .expect("Failed to create raster pipeline");

        let render_target = initializer
            .new_render_target(
                &devices[0],
                Texture2D {
                    width: unsafe { NonZeroU32::new_unchecked(1920) },
                    height: unsafe { NonZeroU32::new_unchecked(1080) },
                    layers: unsafe { NonZeroU32::new_unchecked(1) },
                },
                TextureFormat::standard(
                    TextureComponentFormat::R16G16B16A16,
                    TextureNumericFormat::UNorm,
                )
                .expect("Invalid texture format"),
                unsafe { NonZeroU32::new_unchecked(1) },
                TextureSampleCount::SC1,
                false,
            )
            .expect("Failed to create render target");

        let mut outputs = HashMap::new();
        outputs.insert(
            0,
            OutputAttachment {
                attachment: &render_target,
                view: TextureView::full(false),
                dependency: None,
            },
        );

        let render_pass = initializer
            .new_render_pass(&[SubpassInfo::new(&HashMap::new(), &outputs, None)
                .expect("Failed to create subpass")])
            .expect("Failed to create render pass");

        let draw = initializer
            .new_draw(&render_pass, 0, &pipeline, 6, 1)
            .expect("Failed to create draw");

        Self {
            obj: None,
            print_signal: false,
            action_signal: false,
            vert_shader,
            frag_shader,
            pipeline,
            render_target,
            render_pass,
            draw,
            cursor_mode: false,
            cursor_mode_dirty: false,
            should_exit: false,
        }
    }
}

impl<'s> Layer<'s> for FractalLayer<'s> {
    type SharedData = SharedData<'s>;

    fn tick(
        &mut self,
        state: &mut State<'s, Self::SharedData>,
        shared_data: &mut Self::SharedData,
    ) {
        if self.should_exit {
            state.exit()
        }

        if self.print_signal {
            self.print_signal = false;
            debug!("Vulkan {}", vulkan_api_version());
            debug!("GLFW {}", glfw_version());
            for (i, device) in state.devices.iter().enumerate() {
                debug!("Device {i} name: {}", device.name())
            }
        }

        if self.action_signal {
            if self.obj.is_none() {
                // let desc = hardcore::descriptor![float];
                // let _ = self.obj.insert(
                //     context.devices[0]
                //         .create_vertex_buffer(self, &desc, NonZeroU64::new(1000).unwrap())
                //         .expect("Failed to allocate vertex buffer"),
                // );
            } else {
                self.obj.take();
            }

            self.action_signal = false;
        }

        if self.cursor_mode_dirty {
            self.cursor_mode_dirty = false;
            self.cursor_mode = !self.cursor_mode;
            let cursor_mode = if self.cursor_mode {
                CursorMode::Disabled
            } else {
                CursorMode::Normal
            };

            shared_data
                .window
                .set_cursor_mode(cursor_mode)
                .expect("Failed to set cursor mode")
        }

        // if let Some(x) = &self.obj {
        //     x.clone();
        // }
        // nothing
    }

    fn handle_event(&mut self, event: &Event) -> bool {
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
                let x = self
                    .vert_shader
                    .push_constants()
                    .expect("Failed to get push constant descriptor");
                if let Some(t) = &x {
                    println!("{x:?}");
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
                self.cursor_mode_dirty = true;
            }
            Event::Window {
                event: WindowEvent::Close,
                ..
            } => {
                self.should_exit = true;
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

    let instance = Instance::new(ApplicationDescriptor {
        name: "Hardcore Fractal sample",
        version: Version {
            major: env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
            minor: env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
            patch: env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
        },
    })
    .expect("Failed to initialise library");

    instance
        .run(move |initializer, devices| {
            let shared_data = SharedData::new(initializer, devices);
            let layer = FractalLayer::new(initializer, devices);
            (vec![Box::new(layer)], shared_data)
        })
        .expect("Failed to run main loop");
}
