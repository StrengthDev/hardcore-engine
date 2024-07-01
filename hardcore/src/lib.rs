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
//! in the previous frame;
//! 2. Pushing new layers into the [layer stack], according to calls to [`push_layer`] in the
//! previous frame;
//! 3. [`Event`] handling, each event goes through the [layer stack] ([`Layer::handle_event`] is
//! called), from top to bottom, until the end of the stack or until a layer handles
//! the event (until the call to [`Layer::handle_event`] returns true);
//! 4. Application logic, each [`Layer`] is updated ([`Layer::tick`] is called), from the bottom to
//! the top of the stack;
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
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

use thiserror::Error;
use tokio::runtime::Builder;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tokio::time::Instant;
use tracing::{error, info, info_span, Instrument};

use hardcore_sys::InitParams;

use crate::event::Event;
use crate::layer::{Context, Layer};
use crate::native::vulkan_debug_callback;
use crate::sync::{Mutex, RwLock};

pub mod event;
pub mod input;
pub mod layer;
mod native;
pub mod resource;
mod sync;
pub mod window;

static RUNNING: AtomicBool = AtomicBool::new(false);
static LAYER_TX: RwLock<Option<UnboundedSender<Box<dyn Layer>>>> = RwLock::new(None);
static LAYER_RX: Mutex<Option<UnboundedReceiver<Box<dyn Layer>>>> = Mutex::new(None);
static POP_LAYER_COUNT: AtomicUsize = AtomicUsize::new(0);
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
    Worker,
}

/// A descriptor used to identify an application by its name and version.
#[derive(Clone)]
pub struct ApplicationDescriptor<'a> {
    /// The name of the application.
    pub name: &'a str,

    /// The major version of the application.
    pub major: u32,

    /// The minor version of the application.
    pub minor: u32,

    /// The patch version of the application.
    pub patch: u32,
}

/// An error within the core **Hardcore** functionality.
#[derive(Error, Debug)]
pub enum CoreError {
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

/// Initialise the library context.
///
/// This function must be called before any other library functions may be used.
pub fn init(app: ApplicationDescriptor) -> Result<(), CoreError> {
    let (tx, rx) = unbounded_channel();
    let _ = LAYER_TX.write().insert(tx);
    let _ = LAYER_RX.lock().insert(rx);
    let (tx, rx) = unbounded_channel();
    let _ = EVENT_TX.write().insert(tx);
    let _ = EVENT_RX.lock().insert(rx);

    let c_name = std::ffi::CString::new(app.name)?;

    let descriptor = hardcore_sys::ApplicationDescriptor {
        name: c_name.into_raw(),
        major: app.major,
        minor: app.minor,
        patch: app.patch,
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

    let res: i32 = unsafe { hardcore_sys::init(params) };

    if res < 0 {
        Err(CoreError::System { code: res })
    } else {
        Ok(())
    }
}

/// Terminate the library.
///
/// Doesn't have to be called if the program is exiting.
/// Once this function is called, [`init`] must be called once again before other library functions.
pub fn terminate() -> Result<(), CoreError> {
    let res: i32 = unsafe { hardcore_sys::term() };

    if res < 0 {
        Err(CoreError::System { code: res })
    } else {
        Ok(())
    }
}

/// The main loop function.
///
/// This function **MUST** be called in the *main* thread in a non-async environment.
pub fn run() -> Result<(), CoreError> {
    std::thread::current()
        .name()
        .map_or(true, move |name| name == "main")
        .then_some(())
        .ok_or(CoreError::NotMain)?;

    THREAD_KIND.set(ThreadKind::Main);

    if EVENT_TX.read().is_none() {
        return Err(CoreError::Uninitialised);
    }

    info!("Starting main loop");
    RUNNING.store(true, Ordering::SeqCst);

    let core_rt = Builder::new_multi_thread()
        .thread_name("core")
        .on_thread_start(|| THREAD_KIND.set(ThreadKind::Core))
        .worker_threads(1)
        .enable_all()
        .build()?;
    let core_thread = core_rt.spawn(core_run());

    while RUNNING.load(Ordering::SeqCst) {
        unsafe { hardcore_sys::poll_events() }
    }

    core_rt.block_on(core_thread)??;

    Ok(())
}

/// Loop for application and rendering logic.
async fn core_run() -> Result<(), CoreError> {
    info!("Starting application and rendering logic loop");
    let mut layers = Vec::<Box<dyn Layer>>::new();

    let mut layer_rx = {
        let mut layer_lock = LAYER_RX.lock();
        layer_lock.take().ok_or(CoreError::Uninitialised)?
    };

    let mut event_rx = {
        let mut event_lock = EVENT_RX.lock();
        event_lock.take().ok_or(CoreError::Uninitialised)?
    };

    let worker_count: AtomicUsize = AtomicUsize::new(0);
    let worker_rt = Builder::new_multi_thread()
        .thread_name_fn(move || format!("worker-{}", worker_count.fetch_add(1, Ordering::SeqCst)))
        .on_thread_start(|| THREAD_KIND.set(ThreadKind::Worker))
        .worker_threads(10) // TODO
        .enable_all()
        .build()?;
    let mut last_frame = Instant::now();
    let mut context = Context::new(worker_rt.handle().clone());
    while RUNNING.load(Ordering::SeqCst) {
        let span = info_span!("Frame", frame = context.frame);

        let current_frame = Instant::now();
        let duration = current_frame - last_frame;
        context.delta_time = duration.as_secs_f64();
        last_frame = current_frame;

        async {
            layers.truncate(layers.len() - POP_LAYER_COUNT.swap(0, Ordering::SeqCst));

            while let Ok(layer) = layer_rx.try_recv() {
                layers.push(layer);
            }

            while let Ok(event) = event_rx.try_recv() {
                for layer in layers.iter_mut().rev() {
                    if layer.handle_event(&event).await {
                        break;
                    }
                }
            }

            context.layer_count = layers.len();
            context.current_layer_idx = 0;

            for layer in &mut layers {
                layer.tick(&context).await;
                context.current_layer_idx += 1;
            }

            let res: i32 = unsafe { hardcore_sys::render_tick() };

            if res < 0 {
                RUNNING.store(false, Ordering::SeqCst);
                Err(CoreError::System { code: res })
            } else {
                Ok(())
            }
        }
        .instrument(span)
        .await?;

        if layers.is_empty() {
            RUNNING.store(false, Ordering::SeqCst);
        }

        context.frame += 1;
    }

    layers.clear();

    let res: i32 = unsafe { hardcore_sys::render_finish() };
    if res < 0 {
        return Err(CoreError::System { code: res });
    }

    worker_rt.shutdown_background();

    let mut layer_lock = LAYER_RX.lock();
    let _ = layer_lock.insert(layer_rx);

    let mut event_lock = EVENT_RX.lock();
    let _ = event_lock.insert(event_rx);

    Ok(())
}

/// If the engine is running, stop it and exit out of the main loop ([`run`]).
pub fn stop() {
    RUNNING.store(false, Ordering::SeqCst);
}

/// Submit a new layer to be pushed into the layer stack in the next frame.
///
/// # Parameters
/// * `layer` - The layer to be pushed.
pub fn push_layer(layer: impl Layer + 'static) -> Result<(), CoreError> {
    let guard = LAYER_TX.read();
    if let Some(tx) = guard.as_ref() {
        tx.send(Box::new(layer)).map_err(|e| {
            error!("Failed to send on layer channel ({e})");
            CoreError::Uninitialised
        })?;
        Ok(())
    } else {
        Err(CoreError::Uninitialised)
    }
}

// TODO change pop layer functionality to keep users from popping beyond the current layer

/// Increment the number of layers to be popped in the next frame by 1.
pub fn pop_layer() {
    if RUNNING.load(Ordering::SeqCst) {
        POP_LAYER_COUNT.fetch_add(1, Ordering::SeqCst);
    }
}

/// Increase the number of layers to be popped in the next frame by an arbitrary amount.
///
/// # Parameters
/// * `count` - The additional number of layers to be popped.
pub fn pop_layers(count: usize) {
    if RUNNING.load(Ordering::SeqCst) {
        POP_LAYER_COUNT.fetch_add(count, Ordering::SeqCst);
    }
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
