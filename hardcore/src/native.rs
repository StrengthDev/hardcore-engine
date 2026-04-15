//! This module contains rust functions that get called from within C++ code.
//! Mostly used for logging/tracing.

use std::ffi::{c_char, c_int, c_void, CStr};
use tracing::span::EnteredSpan;
use tracing::{
    debug, debug_span, error, error_span, info, info_span, trace, trace_span, warn, warn_span,
};

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

macro_rules! impl_stateful_fn {
    ($fn_name:ident($($param:ident: $param_type:ty), *)$( -> $return_type:ty)?, $impl_mod:ident) => {

mod $impl_mod {
    use core::ffi::c_void;
    use core::marker::PhantomData;

    pub(super) mod stateful_fn_trait {
        use core::ffi::c_void;

        pub(crate) trait $fn_name {
            fn callback(&self) -> unsafe extern "C" fn($($param_type,)* *mut c_void)$( -> $return_type)?;

            fn user_data(&self) -> *mut c_void;
        }
    }

    pub(super) struct $fn_name<UserCallback: FnMut($($param_type,)*)$( -> $return_type)?> {
        user_data: *mut c_void,
        _callback: PhantomData<UserCallback>,
    }

    impl<UserCallback: FnMut($($param_type,)*)$( -> $return_type)?> $fn_name<UserCallback> {
        pub(super) fn new(callback: UserCallback) -> $fn_name<UserCallback> {
            let boxed = Box::new(callback);
            $fn_name {
                user_data: Box::into_raw(boxed) as *mut c_void,
                _callback: PhantomData,
            }
        }

        unsafe extern "C" fn callback_impl($($param: $param_type,)* user_data: *mut c_void)$( -> $return_type)? {
            let ptr = user_data as *mut UserCallback;
            let fun = &mut *ptr;
            fun($($param,)*)
        }
    }

    impl<UserCallback: FnMut($($param_type,)*)$( -> $return_type)?> stateful_fn_trait::$fn_name for $fn_name<UserCallback> {
        fn callback(&self) -> unsafe extern "C" fn($($param_type,)* *mut c_void)$( -> $return_type)? {
            Self::callback_impl
        }

        fn user_data(&self) -> *mut c_void {
            self.user_data
        }
    }

    impl<UserCallback: FnMut($($param_type,)*)$( -> $return_type)?> Drop for $fn_name<UserCallback> {
        fn drop(&mut self) {
            let ptr = self.user_data as *mut UserCallback;
            let _ = unsafe { Box::from_raw(ptr) };
        }
    }
}

struct $fn_name {
    inner: Box<dyn $impl_mod::stateful_fn_trait::$fn_name>,
}

impl $fn_name {
    fn new(callback: impl FnMut($($param_type,)*)$( -> $return_type)? + 'static) -> $fn_name {
        $fn_name {
            inner: Box::new($impl_mod::$fn_name::new(callback)),
        }
    }

    fn callback(&self) -> unsafe extern "C" fn($($param_type,)* *mut core::ffi::c_void)$( -> $return_type)? {
        self.inner.callback()
    }

    fn user_data(&self) -> *mut core::ffi::c_void {
        self.inner.user_data()
    }

    fn c_ptrs(stateful_fn: &Option<Self>) -> (
        Option<unsafe extern "C" fn($($param_type,)* *mut core::ffi::c_void)$( -> $return_type)?>,
        *mut core::ffi::c_void
    ) {
        if let Some(ref stateful_fn) = stateful_fn {
            (Some(stateful_fn.callback()), stateful_fn.user_data())
        } else {
            (None, std::ptr::null_mut())
        }
    }
}

    };
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;
    use std::rc::Rc;

    impl_stateful_fn!(TestStatefulFunction(x: u32, y: f32) -> f32, test_stateful_function_impl);

    #[test]
    fn stateful_function() {
        let state = Rc::new(RefCell::new(0));

        let state_ref = state.clone();
        let stateful_fn = Some(TestStatefulFunction::new(move |x, y| {
            let mut value = state_ref.borrow_mut();
            *value += 1;
            x as f32 * y
        }));

        let (fn_ptr, fn_data) = TestStatefulFunction::c_ptrs(&stateful_fn);
        let fn_ptr = fn_ptr.expect("Null function pointer");

        let mut ret;
        ret = unsafe { fn_ptr(1, 2.5, fn_data) };

        assert_eq!(*state.borrow(), 1);
        assert_eq!(ret, 2.5);

        {
            let mut value = state.borrow_mut();
            *value = 4;
        }

        ret = unsafe { fn_ptr(5, 1.5, fn_data) };

        assert_eq!(*state.borrow(), 5);
        assert_eq!(ret, 7.5);
    }
}
