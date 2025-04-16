//! Automatically generated **Rust** bindings to **Hardcore**'s native module.

#![warn(missing_docs)]

extern crate link_cplusplus;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

// Having these aliases hardcoded here is really sad, but there's no better way to add the doc comments.
impl MouseButton {
    /// The left mouse button, an alias for mouse button 1.
    pub const LEFT: MouseButton = MouseButton::Button1;

    /// The right mouse button, an alias for mouse button 2.
    pub const RIGHT: MouseButton = MouseButton::Button2;

    /// The middle mouse button, an alias for mouse button 3.
    pub const MIDDLE: MouseButton = MouseButton::Button2;
}
