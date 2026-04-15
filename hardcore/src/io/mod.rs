use window::WindowCall;

use crate::Error;
use tokio::sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender};
use tracing::error;

pub mod event;
pub mod input;
pub mod window;

// Because all GLFW functions must be called from the main thread, but layers are processed on
// another thread, we use a mechanism where functions that result in GLFW calls are packed and sent
// to the main thread for execution.

enum Call {
    Window(WindowCall),
}

impl Call {
    fn execute(self) -> Result<(), Error> {
        match self {
            Call::Window(call) => call.execute()?,
        }

        Ok(())
    }
}

#[derive(Clone)]
pub(super) struct Caller {
    tx: UnboundedSender<Call>,
}

impl Caller {
    /// Submit a call for execution in another thread.
    fn submit(&self, call: Call) -> Result<(), Error> {
        self.tx.send(call).map_err(move |_| Error::Send)
    }
}

pub(super) struct Executor {
    rx: UnboundedReceiver<Call>,
}

impl Executor {
    /// Execute every newly submitted call.
    pub(super) fn flush_execute(&mut self) {
        while let Ok(call) = self.rx.try_recv() {
            if let Err(error) = call.execute() {
                error!("Call execution failed: {error}");
            }
        }
    }
}

pub(super) fn new_context() -> (Caller, Executor) {
    let (tx, rx) = unbounded_channel();

    (Caller { tx }, Executor { rx })
}
