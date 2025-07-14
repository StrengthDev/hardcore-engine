//! Hardcore is a low-level graphics engine implemented using the Vulkan API.
//!
//! While it's designated a graphics engine, there is no GUI, it is used like a library. In the
//! future there may be some utilities to facilitate creating your own level editor and such, but
//! the engine itself will likely never provide such tools directly.
//!
//! ### Still a work in progress!
//!
//! The engine is still extremely early in development so use at your own risk. This project
//! started as learning exercise but will hopefully keep getting developed until it becomes a
//! full-featured graphics engine.
//!
//! # Execution
//!
//! **Hardcore**'s main execution loop is contained within [`run`]. In this loop, execution goes as
//! follows, in order:
//!
//! 1. Popping layers from the [layer stack], according to calls to [`pop_layer`] and [`pop_layers`]
//!    in the previous frame;
//! 2. Pushing new layers into the [layer stack], according to calls to [`push_layer`] in the
//!    previous frame;
//! 3. [`Event`] handling, each event goes through the [layer stack] ([`Layer::handle_event`] is
//!    called), from top to bottom, until the end of the stack or until a layer handles
//!    the event (until the call to [`Layer::handle_event`] returns true);
//! 4. Application logic, each [`Layer`] is updated ([`Layer::tick`] is called), from the bottom to
//!    the top of the stack;
//! 5. Rendering logic and synchronisation;
//!
//! Before starting the main loop,
//! [`init`] should be called to initialize the global environment and then some [`Layer`]\(s\)
//! should be pushed to the global layer stack (see [`push_layer`]).
//!
//! ### Warning
//!
//! Both [`init`] and [`run`] (and [`terminate`]) **MUST** be called from the main thread!
//!
//! [layer stack]: layer

// TODO add example, unless its too long

#![warn(missing_docs)]

use std::cell::Cell;
use std::fmt::{Display, Formatter};
use std::marker::PhantomData;
use std::sync::atomic::{AtomicUsize, Ordering};

use crate::device::Device;
use crate::event::Event;
use crate::layer::Layer;
use crate::native::vulkan_debug_callback;
use crate::sync::{Mutex, RwLock};
use crate::window::GLFWCall;
use context::Context;
use hardcore_sys::InitParams;
use thiserror::Error;
use tokio::runtime::Builder;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tokio::time::Instant;
use tracing::{error, info, info_span};

pub mod context;
mod device;
pub mod event;
pub mod input;
pub mod layer;
mod native;
pub mod render;
pub mod resource;
pub mod shader;
mod sync;
pub mod window;

/// Hardcore's version.
pub static VERSION: Version = Version {
    major: const_unwrap(u32::from_str_radix(env!("CARGO_PKG_VERSION_MAJOR"), 10), 0),
    minor: const_unwrap(u32::from_str_radix(env!("CARGO_PKG_VERSION_MINOR"), 10), 0),
    patch: const_unwrap(u32::from_str_radix(env!("CARGO_PKG_VERSION_PATCH"), 10), 0),
};

const fn const_unwrap(result: Result<u32, std::num::ParseIntError>, default: u32) -> u32 {
    if let Ok(value) = result {
        value
    } else {
        default
    }
}

static EVENT_TX: RwLock<Option<UnboundedSender<Event>>> = RwLock::new(None);
static EVENT_RX: Mutex<Option<UnboundedReceiver<Event>>> = Mutex::new(None);
static GLFW_CALL_TX: RwLock<Option<UnboundedSender<GLFWCall>>> = RwLock::new(None);
static GLFW_CALL_RX: Mutex<Option<UnboundedReceiver<GLFWCall>>> = Mutex::new(None);

thread_local! {static THREAD_KIND: Cell<ThreadKind> = const { Cell::new(ThreadKind::Uninitialised) }}

enum ThreadKind {
    Uninitialised,
    Main,
    Core,
    // Audio,
    // Physics,
    // Networking,
    Worker,
}

/// A version value.
#[derive(Clone, Debug)]
pub struct Version {
    /// The major version.
    pub major: u32,

    /// The minor version.
    pub minor: u32,

    /// The patch version.
    pub patch: u32,
}

impl Display for Version {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_str(format!("v{}.{}.{}", self.major, self.minor, self.patch).as_str())
    }
}

/// A descriptor used to identify an application by its name and version.
#[derive(Clone)]
pub struct ApplicationDescriptor<'a> {
    /// The name of the application.
    pub name: &'a str,

    /// The version of the application.
    pub version: Version,
}

/// An error within the core **Hardcore** functionality.
#[derive(Error, Debug)]
pub enum CoreError {
    /// An error has occurred withing the system crate.
    #[error(transparent)]
    SystemError(#[from] hardcore_sys::Error),

    /// Failed to join with tokio task.
    #[error(transparent)]
    TokioJoin(#[from] tokio::task::JoinError),

    /// Failed to create tokio runtime.
    #[error(transparent)]
    IO(#[from] std::io::Error),

    /// An error as occurred inside the native module.
    #[error("an error as occurred inside the native module (code {code})")]
    System {
        /// The error code returned.
        code: i32,
    },

    /// The function isn't getting executed on the main thread.
    #[error("the function isn't getting executed on the main thread")]
    NotMain,

    /// The context has not been initialised yet.
    #[error("the context has not been initialised yet")]
    Uninitialised,

    /// The provided string could not be converted into a C string.
    #[error("the provided string could not be converted into a C string")]
    InvalidString(#[from] std::ffi::NulError),
}

pub struct Instance {
    /// Dummy member.
    ///
    /// Used to make Instance not [`Send`] and not [`Sync`].
    not_send_sync: PhantomData<*const ()>,
}

impl Instance {
    /// Initialise the library context.
    ///
    /// This function must be called before any other library functions may be used.
    pub fn create(app: ApplicationDescriptor) -> Result<Instance, CoreError> {
        std::thread::current()
            .name()
            .map_or(true, move |name| name == "main")
            .then_some(())
            .ok_or(CoreError::NotMain)?;

        THREAD_KIND.set(ThreadKind::Main);

        info!("hardcore {VERSION}");

        let (tx, rx) = unbounded_channel();
        let _ = EVENT_TX.write().insert(tx);
        let _ = EVENT_RX.lock().insert(rx);

        let (tx, rx) = unbounded_channel();
        let _ = GLFW_CALL_TX.write().insert(tx);
        let _ = GLFW_CALL_RX.lock().insert(rx);

        let c_name = std::ffi::CString::new(app.name)?;

        let descriptor = hardcore_sys::ApplicationDescriptor {
            name: c_name.into_raw(),
            version: hardcore_sys::Version {
                major: app.version.major,
                minor: app.version.minor,
                patch: app.version.patch,
            },
        };

        let render_params = hardcore_sys::RenderParams {
            max_frames_in_flight: 2,
            debug_callback: Some(vulkan_debug_callback),
        };

        let params = InitParams {
            app: descriptor,
            render_params,
            log_fn: Some(native::log),
            start_span_fn: Some(native::start_span),
            end_span_fn: Some(native::end_span),
        };

        unsafe { hardcore_sys::init(params).into_std_result()? };

        Ok(Instance {
            not_send_sync: PhantomData,
        })
    }

    /// The main loop function.
    ///
    /// This function **MUST** be called in the *main* thread in a non-async environment.
    pub fn run(&self, initialize: fn(context: &mut Context)) -> Result<(), CoreError> {
        if EVENT_TX.read().is_none() {
            return Err(CoreError::Uninitialised);
        }

        let mut glfw_call_rx = {
            let mut event_lock = GLFW_CALL_RX.lock();
            event_lock.take().ok_or(CoreError::Uninitialised)?
        };

        info!("Starting main loop");

        let core_rt = Builder::new_multi_thread()
            .thread_name("core")
            .on_thread_start(|| THREAD_KIND.set(ThreadKind::Core))
            .worker_threads(1)
            .enable_all()
            .build()?;

        let core_thread = core_rt.spawn_blocking(move || core_run(initialize));

        while !core_thread.is_finished() {
            while let Ok(call) = glfw_call_rx.try_recv() {
                if call.execute().is_err() {
                    error!("Failed to send GLFW call results to calling thread");
                }
            }

            unsafe { hardcore_sys::poll_events() }
        }

        core_rt.block_on(core_thread)??;

        info!("Main loop finished");

        Ok(())
    }
}

impl Drop for Instance {
    fn drop(&mut self) {
        unsafe { hardcore_sys::term() };
    }
}

/// Loop for application and rendering logic.
fn core_run(initialize: fn(context: &mut Context)) -> Result<(), CoreError> {
    info!("Starting application and rendering logic loop");

    let mut event_rx = {
        let mut event_lock = EVENT_RX.lock();
        event_lock.take().ok_or(CoreError::Uninitialised)?
    };

    let mut context = Context::create(Device::count());
    let mut layers: Vec<Box<dyn Layer>> = vec![];

    initialize(&mut context);

    let worker_count: AtomicUsize = AtomicUsize::new(0);
    let worker_rt = Builder::new_multi_thread()
        .thread_name_fn(move || format!("worker-{}", worker_count.fetch_add(1, Ordering::SeqCst)))
        .on_thread_start(|| THREAD_KIND.set(ThreadKind::Worker))
        .worker_threads(10) // TODO
        .enable_all()
        .build()?;

    context.running = true;
    let mut last_frame = Instant::now();
    while context.running {
        let _span = info_span!("Frame", frame = context.frame).entered();

        let current_frame = Instant::now();
        let duration = current_frame - last_frame;
        context.delta_time = duration.as_secs_f64();
        last_frame = current_frame;

        layers.truncate(layers.len() - context.layer_pop_count);
        context.layer_pop_count = 0;

        layers.append(&mut context.pushed_layers);

        context.layer_count = layers.len();

        while let Ok(event) = event_rx.try_recv() {
            for layer in layers.iter_mut().rev() {
                if layer.handle_event(&mut context, &event) {
                    break;
                }
            }
        }

        context.current_layer_idx = 0;
        let layer_count = layers.len();
        for layer in layers.iter_mut() {
            layer.tick(&mut context);
            context.current_layer_idx += 1;
            if layer_count <= context.current_layer_idx + context.layer_pop_count {
                break;
            }
        }

        let result = unsafe { hardcore_sys::render_tick().into_std_result() };
        if result.is_err() {
            context.running = false;
        }

        if layers.is_empty() {
            context.running = false;
        }

        context.frame += 1;
    }

    layers.clear();

    unsafe { hardcore_sys::render_finish().into_std_result()? };

    worker_rt.shutdown_background();

    let mut event_lock = EVENT_RX.lock();
    let _ = event_lock.insert(event_rx);

    info!("Core thread exiting");

    Ok(())
}

/// Send a new event to the event queue.
///
/// # Parameters
/// * `event` - The event to be sent to the queue.
pub(crate) fn emit_event(event: Event) -> Result<(), CoreError> {
    let guard = EVENT_TX.read();
    if let Some(tx) = guard.as_ref() {
        tx.send(event).map_err(|e| {
            error!("Failed to send on event channel ({e})");
            CoreError::Uninitialised
        })?;
        Ok(())
    } else {
        Err(CoreError::Uninitialised)
    }
}
