//! As **Hardcore** is used to make real-time interactive programs, there is a need to be able to
//! handle various events, either coming for the system, individual [windows][crate::window::Window]
//! or some other source. Thus, **Hardcore** will receive those events and forward them to
//! application defined layers, through [`Layer::handle_event`][crate::layer::Layer::handle_event].
//!
//! All possible events are described by the [`Event`] enum.

use std::path::PathBuf;

use crate::input::{ButtonAction, KeyboardKey, Modifiers, MouseButton};

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

/// A window event's content.
///
/// This type of event is always associated with a specific [`Window`][crate::window::Window].
#[derive(Debug, Clone)]
pub enum WindowEvent {
    /// A window position event.
    ///
    /// This type of event is emitted when the window is moved.
    Position {
        /// The new x-coordinate, in screen coordinates, of the upper-left corner of the content
        /// area of the window.
        x: i32,

        /// The new y-coordinate, in screen coordinates, of the upper-left corner of the content
        /// area of the window.
        y: i32,
    },

    /// A window size event.
    ///
    /// This type of event is emitted when the window is resized.
    Size {
        /// The new width, in screen coordinates, of the window.
        width: i32,

        /// The new height, in screen coordinates, of the window.
        height: i32,
    },

    /// A window close event.
    ///
    /// This type of event is typically emitted, for example, when a user clicks the window's close
    /// button or uses a close command like **Alt** + **F4**.
    Close,

    /// A window refresh event.
    ///
    /// This type of event is typically emitted when the contents of the window need to be
    /// refreshed after getting damaged, like after a resize.
    ///
    /// This mostly used internally to re-create *swapchains* and render targets.
    Refresh,

    /// A window focused event.
    ///
    /// This type of event is emitted when the window comes into or out of focus.
    ///
    /// The inner value is **`true`** if the window has come into focus, and **`false`** if it has
    /// come out of focus.
    Focused(bool),

    /// A window minimized event.
    ///
    /// This type of event is emitted when the window is minimized/iconified or restored.
    ///
    /// The inner value is **`true`** if the window has been minimized, and **`false`** if it has
    /// been restored.
    Minimized(bool),

    /// A window maximized event.
    ///
    /// This type of event is emitted when the window is maximized or restored.
    ///
    /// The inner value is **`true`** if the window has been maximized, and **`false`** if it has
    /// been restored.
    Maximized(bool),

    /// A window framebuffer event.
    ///
    /// This type of event is typically emitted when the window is resized.
    ///
    /// This differs from the [`Size`][WindowEvent::Size] event in some systems where something
    /// like DPI scaling is used, and always gives appropriate dimensions for rendering.
    Framebuffer {
        /// The new width, in pixels, of the framebuffer.
        width: i32,

        /// The new height, in pixels, of the framebuffer.
        height: i32,
    },

    /// A window scale event.
    ///
    /// This type of event is emitted when the window is rescaled.
    Scale {
        /// The new x-axis content scale of the window.
        x: f32,

        /// The new y-axis content scale of the window.
        y: f32,
    },

    /// A mouse button event.
    ///
    /// This type of event is emitted when a mouse button is pressed or released.
    MouseButton {
        /// The mouse button that was pressed or released.
        button: MouseButton,

        /// Either a [`Press`][ButtonAction::Press] or a [`Release`][ButtonAction::Release] action.
        ///
        /// Future releases may add more actions.
        action: ButtonAction,

        /// The currently active [`Modifiers`].
        mods: Modifiers,
    },

    /// A cursor position event.
    ///
    /// This type of event is emitted when the mouse cursor is moved.
    CursorPosition {
        /// The new cursor x-coordinate, relative to the left edge of the content area.
        x: f64,

        /// The new cursor y-coordinate, relative to the top edge of the content area.
        y: f64,
    },

    /// A cursor entered event.
    ///
    /// This type of event is emitted when the mouse cursor enters or leaves the content area.
    ///
    /// The inner value is **`true`** if the cursor has entered the window's content area, and
    /// **`false`** if it has left the content area.
    Entered(bool),

    /// A scroll event.
    ///
    /// This type of event is typically emitted when a user, for example, uses the mouse scroll wheel.
    Scroll {
        /// The scroll offset along the x-axis.
        x_offset: f64,

        /// The scroll offset along the y-axis.
        y_offset: f64,
    },

    /// A keyboard key event.
    ///
    /// This type of event is emitted when a keyboard key is pressed, released or held (repeated).
    Key {
        /// The keyboard key that was pressed, released or repeated.
        key: KeyboardKey,

        /// The platform-specific scancode of the key.
        ///
        /// Probably not relevant for most use cases.
        scan_code: i32,

        /// A [`Press`][ButtonAction::Press], [`Release`][ButtonAction::Release] or a
        /// [`Repeat`][ButtonAction::Repeat] action.
        ///
        /// Future releases may add more actions.
        action: ButtonAction,

        /// The currently active [`Modifiers`].
        mods: Modifiers,
    },

    /// A character event.
    ///
    /// This type of event is emitted when a character key is pressed, released or held
    /// (repeated), essentially, when something is typed.
    ///
    /// The inner value is the character which was typed.
    Char(char),

    /// A character with modifiers event.
    ///
    /// This type of event is emitted when a character key is pressed, released or held
    /// (repeated), essentially, when something is typed.
    CharMods {
        /// The character which was typed.
        char: char,

        /// The currently active [`Modifiers`].
        mods: Modifiers,
    },

    /// A path drop event.
    ///
    /// This type of event is emitted when selection is dropped inside the content area.
    ///
    /// The inner value is a vector containing the paths which were dropped.
    Drop(Vec<PathBuf>),
}
