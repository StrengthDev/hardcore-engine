use crate::layer::Layer;
use std::ffi::CStr;
use std::num::NonZeroU64;

pub struct Device {
    id: u32,
}

impl Device {
    pub(crate) fn new(id: u32) -> Self {
        Self { id }
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
    pub fn create_window<'c>(
        &self,
        #[allow(unused_variables)] layer: &dyn Layer<'c>,
        width: u32,
        height: u32,
        pos_x: Option<i32>,
        pos_y: Option<i32>,
        name: &str,
    ) -> Result<crate::window::Window<'c>, crate::window::WindowError> {
        crate::window::Window::create(self.id, width, height, pos_x, pos_y, name)
    }

    pub fn create_vertex_buffer<'c>(
        &self,
        #[allow(unused_variables)] layer: &dyn Layer<'c>,
        descriptor: &crate::resource::descriptor::Descriptor,
        count: NonZeroU64,
    ) -> Result<crate::resource::VertexBuffer<'c, false>, crate::resource::buffer::BufferError>
    {
        crate::resource::VertexBuffer::<'c, false>::create(self.id, descriptor, count)
    }
}
