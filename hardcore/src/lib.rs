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

pub use device::Device;
use event::Event;
use layer::Layer;
pub use meta::{Version, VERSION};
use native::vulkan_debug_callback;
use state::State;
use sync::{Mutex, RwLock};

use hardcore_sys::InitParams;

use std::cell::Cell;
use std::marker::PhantomData;
use std::rc::Rc;
use thiserror::Error;
use tokio::runtime::Builder;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tracing::{error, info, info_span};

pub mod allocator;
pub mod device;
pub mod event;
mod handle;
pub mod io;
pub mod layer;
pub mod meta;

#[macro_use]
mod native;
pub mod color;
pub mod ops;
pub mod resource;
pub mod shader;
pub mod state;
mod sync;

static EVENT_TX: RwLock<Option<UnboundedSender<Event>>> = RwLock::new(None);
static EVENT_RX: Mutex<Option<UnboundedReceiver<Event>>> = Mutex::new(None);

thread_local! {static THREAD_KIND: Cell<ThreadKind> = const { Cell::new(ThreadKind::Uninitialised) }}

enum ThreadKind {
    Uninitialised,
    Main,
    Core,
    // Audio,
    // Physics,
    // Networking,
    // Worker,
    // Loader,
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
pub enum Error {
    /// An error has occurred withing the system crate.
    #[error(transparent)]
    System(#[from] hardcore_sys::Error),

    /// Failed to join with tokio task.
    #[error(transparent)]
    TokioJoin(#[from] tokio::task::JoinError),

    /// Failed to send event.
    #[error(transparent)]
    TokioSend(#[from] tokio::sync::mpsc::error::SendError<Event>),

    /// Failed to create tokio runtime.
    #[error(transparent)]
    IO(#[from] std::io::Error),

    /// The function isn't getting executed on the main thread.
    #[error("the function isn't getting executed on the main thread")]
    NotMain,

    /// The context has not been initialized yet.
    #[error("the context has not been initialised yet")]
    Uninitialised,

    /// The provided string could not be converted into a C string.
    #[error("the provided string could not be converted into a C string")]
    InvalidString(#[from] std::ffi::NulError),

    /// Unknown shader stage.
    #[error("Could not infer shader stage from file extension \"{0}\"")]
    UnknownStage(String),

    /// Failed to acquire glslang compiler.
    #[cfg(feature = "shader-compilation")]
    #[error("Failed to acquire glslang compiler")]
    NoCompiler,

    /// GLSLang compiler error.
    #[cfg(feature = "shader-compilation")]
    #[error(transparent)]
    GLSLang(#[from] glslang::error::GlslangError),

    /// Failed to submit call to another thread.
    #[error("failed to submit call to another thread")]
    Send,

    /// Failed to execute call in another thread.
    #[error("failed to execute call in another thread")]
    Execute,

    /// Failed to receive call execution result from another thread.
    #[error("failed to receive call execution result from another thread")]
    ReceiveResult,

    /// Invalid index type.
    #[error("Invalid index type")]
    Index,

    /// The specified texture format does not exist.
    #[error("The specified texture format does not exist")]
    FormatDoesNotExist,

    /// The specified input parameters are not valid.
    #[error("The specified input parameters are not valid: {0}")]
    InvalidParams(String),

    /// The native library has provided an unexpected value.
    #[error("The native library has provided an unexpected value")]
    UnexpectedValue,
}

pub struct Initializer {
    /// Dummy member to keep this from being constructed by users.
    _pvt: (),
}

impl<'a> allocator::seal::Seal for Initializer {}

impl<'a> allocator::Allocator<'a> for Initializer {}

pub struct Instance {
    /// Dummy member.
    ///
    /// Used to make Instance not [`Send`] and not [`Sync`].
    _not_send_sync: PhantomData<*const ()>,
}

impl Instance {
    /// Initialise the library context.
    ///
    /// This function must be called before any other library functions may be used.
    pub fn new(app: ApplicationDescriptor) -> Result<Instance, Error> {
        std::thread::current()
            .name()
            .is_none_or(move |name| name == "main")
            .then_some(())
            .ok_or(Error::NotMain)?;

        THREAD_KIND.set(ThreadKind::Main);

        info!("hardcore {VERSION}");

        let (tx, rx) = unbounded_channel();
        let _ = EVENT_TX.write().insert(tx);
        let _ = EVENT_RX.lock().insert(rx);

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
            _not_send_sync: PhantomData,
        })
    }

    /// The main loop function.
    ///
    /// This function **MUST** be called in the *main* thread in a non-async environment.
    pub fn run<'s, InitializeFn, SharedData>(&self, initialize: InitializeFn) -> Result<(), Error>
    where
        InitializeFn: FnOnce(
                &'s Initializer,
                &Rc<Vec<Device>>,
            ) -> (
                Vec<Box<dyn Layer<'s, SharedData = SharedData> + 's>>,
                SharedData,
            ) + Send
            + 'static,
    {
        if EVENT_TX.read().is_none() {
            return Err(Error::Uninitialised);
        }

        let (io_caller, mut io_executor) = io::new_context();

        info!("Starting main loop");

        let core_rt = Builder::new_multi_thread()
            .thread_name("core")
            .on_thread_start(|| THREAD_KIND.set(ThreadKind::Core))
            .worker_threads(1)
            .enable_all()
            .build()?;

        let core_thread = core_rt.spawn_blocking(move || core_run(io_caller, initialize));

        while !core_thread.is_finished() {
            io_executor.flush_execute();

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

static LAYER_INITIALIZER: Initializer = Initializer { _pvt: () };

/// Loop for application and rendering logic.
fn core_run<'s, InitializeFn, SharedData>(
    io_caller: io::Caller,
    initialize: InitializeFn,
) -> Result<(), Error>
where
    InitializeFn: FnOnce(
            &'s Initializer,
            &Rc<Vec<Device>>,
        ) -> (
            Vec<Box<dyn Layer<'s, SharedData = SharedData> + 's>>,
            SharedData,
        ) + Send,
{
    info!("Starting application and rendering logic loop");

    let mut event_rx = {
        let mut event_lock = EVENT_RX.lock();
        event_lock.take().ok_or(Error::Uninitialised)?
    };

    let mut state = State::new(Device::count(), io_caller);

    let (mut layers, mut shared_data) = initialize(&LAYER_INITIALIZER, &state.devices);

    state.running = true;
    while state.running {
        let _span = info_span!("Frame", frame = state.frame).entered();

        while let Ok(event) = event_rx.try_recv() {
            for layer in layers.iter_mut().rev() {
                if layer.handle_event(&event) {
                    break;
                }
            }
        }

        state.current_layer_idx = 0;
        let layer_count = layers.len();
        for layer in layers.iter_mut() {
            layer.tick(&mut state, &mut shared_data);
            state.current_layer_idx += 1;
            if layer_count <= state.current_layer_idx + state.layer_pop_count {
                break;
            }
        }

        if unsafe { hardcore_sys::render_tick().into_std_result() }.is_err() {
            state.running = false;
            break;
        }

        state.tick();
        state.update_layers(&mut layers);
    }

    layers.clear();
    drop(shared_data);

    unsafe { hardcore_sys::render_finish().into_std_result()? };

    let mut event_lock = EVENT_RX.lock();
    let _ = event_lock.insert(event_rx);

    info!("Core thread exiting");

    Ok(())
}

/// Send a new event to the event queue.
///
/// # Parameters
/// * `event` - The event to be sent to the queue.
pub(crate) fn emit_event(event: Event) -> Result<(), Error> {
    let guard = EVENT_TX.read();
    if let Some(tx) = guard.as_ref() {
        tx.send(event)?;
        Ok(())
    } else {
        Err(Error::Uninitialised)
    }
}
