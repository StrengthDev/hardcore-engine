use crate::layer::Layer;
use std::ffi::CStr;
use std::num::NonZeroU64;

pub struct Device {
    id: u32,
    io_caller: crate::io::Caller,
}

impl Device {
    pub(crate) fn new(id: u32, io_caller: crate::io::Caller) -> Self {
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
    /// Instantiate a new window.
    pub fn create_window<'l>(
        &self,
        #[allow(unused_variables)] layer: &dyn Layer<'l>,
        width: u32,
        height: u32,
        pos_x: Option<i32>,
        pos_y: Option<i32>,
        name: &str,
    ) -> Result<crate::io::window::Window<'l>, crate::io::window::WindowError> {
        crate::io::window::Window::create(
            self.io_caller.clone(),
            self.id,
            width,
            height,
            pos_x,
            pos_y,
            name,
        )
    }

    pub fn create_vertex_buffer<'l>(
        &self,
        #[allow(unused_variables)] layer: &dyn Layer<'l>,
        descriptor: &crate::resource::descriptor::Descriptor,
        count: NonZeroU64,
    ) -> Result<crate::resource::VertexBuffer<'l, false>, crate::resource::buffer::BufferError>
    {
        crate::resource::VertexBuffer::<'l, false>::create(self.id, descriptor, count)
    }
}
