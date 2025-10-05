//! This module describes various kinds of input received from an application user.

use std::fmt::{Debug, Formatter};

pub use hardcore_sys::{ButtonAction, KeyboardKey, MouseButton};

/// The modifiers of a button input.
#[derive(Eq, PartialEq, Copy, Clone)]
pub struct Modifiers {
    /// Integer value holding set bit flags representing each individual modifier.
    flags: i32,
}

impl Modifiers {
    /// Create a new modifiers instance from an already existing flag bits value.
    pub(crate) fn from(flags: i32) -> Self {
        Self { flags }
    }

    fn check_flag(&self, flag: i32) -> bool {
        (self.flags & flag) != 0
    }

    /// Return true if one or more Shift keys were held down.
    pub fn shift(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_SHIFT)
    }

    /// Return true if one or more Control keys were held down.
    pub fn ctrl(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_CONTROL)
    }

    /// Return true if one or more Alt keys were held down.
    pub fn alt(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_ALT)
    }

    /// Return true if one or more Super keys were held down.
    pub fn command(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_SUPER)
    }

    /// Return true if he Caps Lock key is enabled.
    pub fn caps_lock(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_CAPS_LOCK)
    }

    /// Return true if the Num Lock key is enabled.
    pub fn num_lock(&self) -> bool {
        self.check_flag(hardcore_sys::MOD_NUM_LOCK)
    }
}

impl Debug for Modifiers {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let mut flags = Vec::with_capacity(6);

        if self.shift() {
            flags.push("Shift")
        }

        if self.ctrl() {
            flags.push("Ctrl")
        }

        if self.alt() {
            flags.push("Alt")
        }

        if self.command() {
            flags.push("Super")
        }

        if self.caps_lock() {
            flags.push("Caps lock")
        }

        if self.num_lock() {
            flags.push("Num lock")
        }

        write!(f, "({})", flags.join("|"))
    }
}
