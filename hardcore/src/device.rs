use std::ffi::CStr;

pub struct Device {
    pub(super) id: u32,
    pub(super) io_caller: crate::io::Caller,
}

impl Device {
    pub(super) fn new(id: u32, io_caller: crate::io::Caller) -> Self {
        Self { id, io_caller }
    }

    pub fn count() -> u32 {
        unsafe { hardcore_sys::device_count() }
    }

    pub fn name(&self) -> &str {
        let name_ptr = unsafe { hardcore_sys::device_name(self.id) };
        if name_ptr.is_null() {
            unreachable!(
                "Device instances should only ever exist within valid contexts and hold valid ids"
            )
        }

        let c_str = unsafe { CStr::from_ptr(name_ptr) };
        c_str.to_str().unwrap_or("UNREADABLE_DEVICE_NAME")
    }
}
