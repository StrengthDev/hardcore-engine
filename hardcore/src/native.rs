use hardcore_sys::LogKind;
use std::ffi::{c_char, c_void, CStr};
use tracing::span::EnteredSpan;
use tracing::{
    debug, debug_span, error, error_span, info, info_span, trace, trace_span, warn, warn_span,
};

const TARGET: &str = "hardcore-sys";

pub(super) unsafe extern "C" fn log(level: LogKind, text: *const c_char) {
    let text = unsafe {
        // SAFETY: `text` is a NUL-terminated C String.
        CStr::from_ptr(text)
    };
    let text = text.to_string_lossy();

    match level {
        LogKind::Trace => trace!(target: TARGET, "{text}"),
        LogKind::Debug => debug!(target: TARGET, "{text}"),
        LogKind::Info => info!(target: TARGET, "{text}"),
        LogKind::Warn => warn!(target: TARGET, "{text}"),
        LogKind::Error => error!(target: TARGET, "{text}"),
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
        LogKind::Trace => trace_span!(target: TARGET, "", span),
        LogKind::Debug => debug_span!(target: TARGET, "", span),
        LogKind::Info => info_span!(target: TARGET, "", span),
        LogKind::Warn => warn_span!(target: TARGET, "", span),
        LogKind::Error => error_span!(target: TARGET, "", span),
    };
    Box::into_raw(Box::new(span.entered())) as *mut c_void
}

pub(super) unsafe extern "C" fn end_span(ptr: *mut c_void) {
    let ptr: *mut EnteredSpan = ptr as *mut EnteredSpan;
    let _ = unsafe { Box::from_raw(ptr) };
}
