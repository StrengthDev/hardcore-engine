use crate::input::{ButtonAction, KeyboardKey, Modifiers, MouseButton};
use std::path::PathBuf;

#[derive(Debug)]
pub enum Event {
    Window { id: usize, event: WindowEvent },
}

#[derive(Debug)]
pub enum WindowEvent {
    Position {
        x: i32,
        y: i32,
    },
    Size {
        width: i32,
        height: i32,
    },
    Close,
    Refresh,
    Focused(bool),
    Minimized(bool),
    Maximized(bool),
    Framebuffer {
        width: i32,
        height: i32,
    },
    Scale {
        x: f32,
        y: f32,
    },
    MouseButton {
        button: MouseButton,
        action: ButtonAction,
        mods: Modifiers,
    },
    CursorPosition {
        x: f64,
        y: f64,
    },
    Entered(bool),
    Scroll {
        x_offset: f64,
        y_offset: f64,
    },
    Key {
        key: KeyboardKey,
        scan_code: i32,
        action: ButtonAction,
        mods: Modifiers,
    },
    Char(char),
    CharMods {
        char: char,
        mods: Modifiers,
    },
    Drop(Vec<PathBuf>),
}
