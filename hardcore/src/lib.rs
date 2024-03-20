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

// TODO add example, unless its too long

#![warn(missing_docs)]

use std::cell::Cell;
use std::sync::atomic::{AtomicBool, Ordering};

use thiserror::Error;
use tokio::runtime::Builder;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tracing::{error, info};

use hardcore_sys::InitParams;

use crate::event::Event;
use crate::layer::{Context, Layer};
use crate::sync::{Mutex, RwLock};

pub mod event;
pub mod input;
pub mod layer;
mod native;
mod sync;
pub mod window;

static RUNNING: AtomicBool = AtomicBool::new(false);
static LAYER_TX: RwLock<Option<UnboundedSender<LayerOp>>> = RwLock::new(None);
static LAYER_RX: Mutex<Option<UnboundedReceiver<LayerOp>>> = Mutex::new(None);
static EVENT_TX: RwLock<Option<UnboundedSender<Event>>> = RwLock::new(None);
static EVENT_RX: Mutex<Option<UnboundedReceiver<Event>>> = Mutex::new(None);

thread_local! {static THREAD_KIND: Cell<ThreadKind> = const { Cell::new(ThreadKind::Uninitialised) }}

enum ThreadKind {
    Uninitialised,
    Main,
    Core,
    Physics,
    Networking,
    Worker,
}

enum LayerOp {
    Push(Box<dyn Layer>),
    Pop,
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
}

/// Initialise the library context.
///
/// This function must be called before any other library functions may be used.
pub fn init() -> Result<(), CoreError> {
    let (tx, rx) = unbounded_channel();
    let _ = LAYER_TX.write().insert(tx);
    let _ = LAYER_RX.lock().insert(rx);
    let (tx, rx) = unbounded_channel();
    let _ = EVENT_TX.write().insert(tx);
    let _ = EVENT_RX.lock().insert(rx);

    let params = InitParams {
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

    let mut layer_lock = LAYER_RX.lock();
    let layer_rx = layer_lock.as_mut().ok_or(CoreError::Uninitialised)?;

    let mut event_lock = EVENT_RX.lock();
    let event_rx = event_lock.as_mut().ok_or(CoreError::Uninitialised)?;

    let mut context = Context::new();
    while RUNNING.load(Ordering::SeqCst) {
        while let Ok(layer_op) = layer_rx.try_recv() {
            match layer_op {
                LayerOp::Push(layer) => layers.push(layer),
                LayerOp::Pop => layers.truncate(layers.len() - 1),
            }
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
            return Err(CoreError::System { code: res });
        }
    }

    Ok(())
}

/// If the engine is running, stop it and exit out of the main loop ([`run`]).
pub fn stop() {
    RUNNING.store(false, Ordering::SeqCst);
}

pub fn push_layer(layer: impl Layer + 'static) -> Result<(), CoreError> {
    let guard = LAYER_TX.read();
    if let Some(tx) = guard.as_ref() {
        tx.send(LayerOp::Push(Box::new(layer))).map_err(|e| {
            error!("Failed to send on layer channel ({e})");
            CoreError::Uninitialised
        })?;
        Ok(())
    } else {
        Err(CoreError::Uninitialised)
    }
}

pub fn pop_layer() -> Result<(), CoreError> {
    let guard = LAYER_TX.read();
    if let Some(tx) = guard.as_ref() {
        tx.send(LayerOp::Pop).map_err(|e| {
            error!("Failed to send on layer channel ({e})");
            CoreError::Uninitialised
        })?;
        Ok(())
    } else {
        Err(CoreError::Uninitialised)
    }
}

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
