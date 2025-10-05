//! As **Hardcore** is used to make real-time interactive programs, there is a need to be able to
//! handle various events, either coming from the system, individual [windows][crate::window::Window]
//! or some other source. Thus, **Hardcore** will receive those events and forward them to
//! application defined layers, through [`Layer::handle_event`][crate::layer::Layer::handle_event].
//!
//! All possible events are described by the [`Event`] enum.

pub use crate::io::event::WindowEvent;

/// A system event.
#[derive(Debug)]
pub enum Event {
    /// An event related to a [`Window`][crate::window::Window].
    Window {
        /// The id of the [`Window`][crate::window::Window] from which the event originates.
        id: usize,

        /// The window event's content.
        event: WindowEvent,
    },
}
