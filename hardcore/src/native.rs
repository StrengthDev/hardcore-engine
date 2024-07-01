use std::ffi::{c_char, c_int, c_void, CStr};

use tracing::{
    debug, debug_span, error, error_span, info, info_span, trace, trace_span, warn, warn_span,
};
use tracing::span::EnteredSpan;

use hardcore_sys::{LogKind, VK_DEVICE_ADDRESS_BINDING, VK_GENERAL, VK_PERFORMANCE, VK_VALIDATION};

const TARGET_SYS: &str = "hardcore-sys";
const TARGET_VK: &str = "vulkan";

pub(super) unsafe extern "C" fn log(level: LogKind, text: *const c_char) {
    let text = unsafe {
        // SAFETY: `text` is a NUL-terminated C String.
        CStr::from_ptr(text)
    };
    let text = text.to_string_lossy();

    match level {
        LogKind::Trace => trace!(target: TARGET_SYS, "{text}"),
        LogKind::Debug => debug!(target: TARGET_SYS, "{text}"),
        LogKind::Info => info!(target: TARGET_SYS, "{text}"),
        LogKind::Warn => warn!(target: TARGET_SYS, "{text}"),
        LogKind::Error => error!(target: TARGET_SYS, "{text}"),
    }
}

pub(super) unsafe extern "C" fn vulkan_debug_callback(
    level: LogKind,
    scope: c_int,
    text: *const c_char,
) {
    let text = unsafe {
        // SAFETY: `text` is a NUL-terminated C String.
        CStr::from_ptr(text)
    };
    let text = text.to_string_lossy();

    let general = if 0 < scope & VK_GENERAL { 'G' } else { '-' };
    let validation = if 0 < scope & VK_VALIDATION { 'V' } else { '-' };
    let performance = if 0 < scope & VK_PERFORMANCE { 'P' } else { '-' };
    let binding = if 0 < scope & VK_DEVICE_ADDRESS_BINDING {
        'B'
    } else {
        '-'
    };

    match level {
        LogKind::Trace => {
            trace!(target: TARGET_VK, "[{general}{validation}{performance}{binding}] {text}")
        }
        LogKind::Debug => {
            debug!(target: TARGET_VK, "[{general}{validation}{performance}{binding}] {text}")
        }
        LogKind::Info => {
            info!(target: TARGET_VK, "[{general}{validation}{performance}{binding}] {text}")
        }
        LogKind::Warn => {
            warn!(target: TARGET_VK, "[{general}{validation}{performance}{binding}] {text}")
        }
        LogKind::Error => {
            error!(target: TARGET_VK, "[{general}{validation}{performance}{binding}] {text}")
        }
    }
}

// TODO as of 3/10/2024, the tracing crate does not support dynamic metadata, so this is the best that can be done for spans

pub(super) unsafe extern "C" fn start_span(level: LogKind, name: *const c_char) -> *mut c_void {
    let name = unsafe {
        // SAFETY: `name` is a NUL-terminated C String.
        CStr::from_ptr(name)
    };
    let span = name.to_string_lossy().to_string();

    let span = match level {
        LogKind::Trace => trace_span!(target: TARGET_SYS, "", span),
        LogKind::Debug => debug_span!(target: TARGET_SYS, "", span),
        LogKind::Info => info_span!(target: TARGET_SYS, "", span),
        LogKind::Warn => warn_span!(target: TARGET_SYS, "", span),
        LogKind::Error => error_span!(target: TARGET_SYS, "", span),
    };
    Box::into_raw(Box::new(span.entered())) as *mut c_void
}

pub(super) unsafe extern "C" fn end_span(ptr: *mut c_void) {
    let ptr: *mut EnteredSpan = ptr as *mut EnteredSpan;
    let _ = unsafe { Box::from_raw(ptr) };
}
