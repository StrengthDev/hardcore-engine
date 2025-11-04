use crate::device::Device;

use std::num::NonZeroU64;

pub(super) mod seal {
    pub trait Seal {}
}

pub trait Allocator<'a>: seal::Seal {
    /// Instantiate a new window.
    fn create_window(
        &'a self,
        device: &Device,
        width: u32,
        height: u32,
        pos_x: Option<i32>,
        pos_y: Option<i32>,
        name: &str,
    ) -> Result<crate::io::window::Window<'a>, crate::io::window::WindowError> {
        crate::io::window::Window::create(
            device.io_caller.clone(),
            device.id,
            width,
            height,
            pos_x,
            pos_y,
            name,
        )
    }

    fn create_vertex_buffer(
        &'a self,
        device: &Device,
        descriptor: &crate::resource::descriptor::Descriptor,
        count: NonZeroU64,
    ) -> Result<crate::resource::VertexBuffer<'a, false>, crate::resource::buffer::BufferError>
    {
        crate::resource::VertexBuffer::<'a, false>::create(device.id, descriptor, count)
    }
}
