//! Cross-platform window management.

// TODO add example

use hardcore_sys::{
    destroy_window, new_window, set_window_char_callback, set_window_char_mods_callback,
    set_window_close_callback, set_window_cursor_enter_callback,
    set_window_cursor_position_callback, set_window_drop_callback, set_window_focus_callback,
    set_window_framebuffer_callback, set_window_key_callback, set_window_maximize_callback,
    set_window_minimize_callback, set_window_mouse_button_callback, set_window_position_callback,
    set_window_refresh_callback, set_window_scale_callback, set_window_scroll_callback,
    set_window_size_callback,
};
use std::ffi::{c_int, CString, NulError};
use std::ptr::addr_of_mut;
use thiserror::Error;

/// An error related to a [`Window`].
#[derive(Error, Debug)]
pub enum WindowError {
    /// Failed to initialise window, this may indicate that the context has not been initialised yet.
    #[error("failed to create a new window")]
    Initialisation,
    /// Could turn provided string slice into a valid [`CString`].
    #[error("could turn provided string slice into a valid C string")]
    InvalidName(#[from] NulError),
}

/// A high-level abstraction over an *OS* window.
pub struct Window {
    handle: hardcore_sys::Window,
}

impl Window {
    /// Instantiate a new window.
    pub fn new(
        width: u32,
        height: u32,
        pos_x: Option<i32>,
        pos_y: Option<i32>,
        name: &str,
    ) -> Result<Self, WindowError> {
        let c_name = CString::new(name)?;

        let params = hardcore_sys::WindowParams {
            width,
            height,
            pos_x: pos_x.unwrap_or(i32::MAX) as c_int,
            pos_y: pos_y.unwrap_or(i32::MAX) as c_int,
            name: c_name.as_ptr(),
        };

        let handle = unsafe {
            let mut handle = new_window(params);

            if handle.handle.is_null() {
                return Err(WindowError::Initialisation);
            }

            let ptr = addr_of_mut!(handle);
            set_window_position_callback(ptr, Some(callback::position));
            set_window_size_callback(ptr, Some(callback::size));
            set_window_close_callback(ptr, Some(callback::close));
            set_window_refresh_callback(ptr, Some(callback::refresh));
            set_window_focus_callback(ptr, Some(callback::focus));
            set_window_minimize_callback(ptr, Some(callback::minimize));
            set_window_maximize_callback(ptr, Some(callback::maximized));
            set_window_framebuffer_callback(ptr, Some(callback::framebuffer));
            set_window_scale_callback(ptr, Some(callback::scale));
            set_window_mouse_button_callback(ptr, Some(callback::mouse_button));
            set_window_cursor_position_callback(ptr, Some(callback::cursor_position));
            set_window_cursor_enter_callback(ptr, Some(callback::entered));
            set_window_scroll_callback(ptr, Some(callback::scroll));
            set_window_key_callback(ptr, Some(callback::key));
            set_window_char_callback(ptr, Some(callback::char));
            set_window_char_mods_callback(ptr, Some(callback::char_mods));
            set_window_drop_callback(ptr, Some(callback::drop));
            handle
        };

        Ok(Self { handle })
    }

    /// Return this window's id.
    ///
    /// This can be useful to identify which window an event comes from.
    pub fn id(&self) -> usize {
        self.handle.id
    }
}

impl Drop for Window {
    fn drop(&mut self) {
        let ptr = addr_of_mut!(self.handle);
        unsafe {
            destroy_window(ptr);
        }
    }
}

unsafe impl Send for Window {}

mod callback {
    use crate::emit_event;
    use crate::event::{Event, WindowEvent};
    use crate::input::{ButtonAction, KeyboardKey, Modifiers, MouseButton};
    use std::ffi::{c_char, c_int, c_uint, CStr};
    use std::path::PathBuf;
    use tracing::error;

    pub(super) unsafe extern "C" fn position(id: usize, x: c_int, y: c_int) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Position { x, y },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn size(id: usize, width: c_int, height: c_int) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Size { width, height },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn close(id: usize) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Close,
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn refresh(id: usize) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Refresh,
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn focus(id: usize, focused: bool) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Focused(focused),
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn minimize(id: usize, minimized: bool) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Minimized(minimized),
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn maximized(id: usize, maximized: bool) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Maximized(maximized),
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn framebuffer(id: usize, width: c_int, height: c_int) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Framebuffer { width, height },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn scale(id: usize, x_scale: f32, y_scale: f32) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Scale {
                x: x_scale,
                y: y_scale,
            },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    #[allow(improper_ctypes_definitions)] // Doesn't matter here
    pub(super) unsafe extern "C" fn mouse_button(
        id: usize,
        button: hardcore_sys::MouseButton,
        action: hardcore_sys::ButtonAction,
        mods: c_int,
    ) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::MouseButton {
                button: MouseButton::from(button),
                action: ButtonAction::from(action),
                mods: Modifiers::from(mods),
            },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn cursor_position(id: usize, x: f64, y: f64) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::CursorPosition { x, y },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn entered(id: usize, entered: bool) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Entered(entered),
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn scroll(id: usize, x_offset: f64, y_offset: f64) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Scroll { x_offset, y_offset },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    #[allow(improper_ctypes_definitions)] // Doesn't matter here
    pub(super) unsafe extern "C" fn key(
        id: usize,
        key: hardcore_sys::KeyboardKey,
        scan_code: i32,
        action: hardcore_sys::ButtonAction,
        mods: c_int,
    ) {
        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Key {
                key: KeyboardKey::from(key),
                scan_code,
                action: ButtonAction::from(action),
                mods: Modifiers::from(mods),
            },
        }) {
            error!("Failed to emit event ({e})")
        }
    }

    pub(super) unsafe extern "C" fn char(id: usize, code_point: c_uint) {
        if let Ok(c) = char::try_from(code_point) {
            if let Err(e) = emit_event(Event::Window {
                id,
                event: WindowEvent::Char(c),
            }) {
                error!("Failed to emit event ({e})")
            }
        } else {
            error!("Failed to parse character from code point")
        }
    }

    pub(super) unsafe extern "C" fn char_mods(id: usize, code_point: c_uint, mods: c_int) {
        if let Ok(c) = char::try_from(code_point) {
            if let Err(e) = emit_event(Event::Window {
                id,
                event: WindowEvent::CharMods {
                    char: c,
                    mods: Modifiers::from(mods),
                },
            }) {
                error!("Failed to emit event ({e})")
            }
        } else {
            error!("Failed to parse character from code point")
        }
    }

    pub(super) unsafe extern "C" fn drop(
        id: usize,
        path_count: c_int,
        paths_ptr: *mut *const c_char,
    ) {
        let count = if let Ok(count) = usize::try_from(path_count) {
            count
        } else {
            error!("Negative path count");
            return;
        };

        let mut paths = Vec::with_capacity(count);
        for idx in 0..count {
            let path_ptr = *paths_ptr.add(idx);
            let c_str = CStr::from_ptr(path_ptr);
            let buf = PathBuf::from(c_str.to_string_lossy().as_ref());

            if !buf.exists() {
                error!("Dropped path does not exist or is inaccessible ({buf:?})");
                continue;
            }

            paths.push(buf)
        }

        if let Err(e) = emit_event(Event::Window {
            id,
            event: WindowEvent::Drop(paths),
        }) {
            error!("Failed to emit event ({e})")
        }
    }
}
