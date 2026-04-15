use crate::resource::buffer::{
    Buffer, CBuffer, CDynamicBuffer, DynamicBuffer, LayoutBuffer, MappedSlice,
    ShaderWritableBuffer, VertexBufferLike,
};
use crate::resource::descriptor::Descriptor;
use crate::resource::Synchronization;
use crate::Error;

use std::num::NonZeroU64;

pub struct VertexBuffer<'s, const WRITABLE: bool> {
    inner: CBuffer<'s>,
}

impl<'s> VertexBuffer<'s, false> {
    pub(crate) fn new<'d>(
        device: u32,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
    ) -> Result<VertexBuffer<'s, false>, Error>
    where
        's: 'd,
    {
        Ok(VertexBuffer::<'s, false> {
            inner: CBuffer::new(
                device,
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                false,
            )?,
        })
    }
}

impl<'s> VertexBuffer<'s, true> {
    pub(crate) fn new<'d>(
        device: u32,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
        synchronization: Synchronization,
    ) -> Result<VertexBuffer<'s, true>, Error>
    where
        's: 'd,
    {
        Ok(VertexBuffer::<'s, true> {
            inner: CBuffer::new(
                device,
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                true,
            )?,
        })
    }
}

impl<'s, const WRITABLE: bool> Buffer<'s> for VertexBuffer<'s, WRITABLE> {
    fn id(&self) -> u64 {
        self.inner.id()
    }
}

impl<'s, const WRITABLE: bool> LayoutBuffer<'s> for VertexBuffer<'s, WRITABLE> {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl<'s, const WRITABLE: bool> VertexBufferLike<'s> for VertexBuffer<'s, WRITABLE> {}

impl<'s> ShaderWritableBuffer<'s> for VertexBuffer<'s, true> {}

pub struct DynamicVertexBuffer<'s> {
    inner: CDynamicBuffer<'s>,
}

impl<'s> DynamicVertexBuffer<'s> {
    pub(crate) fn new(
        device: u32,
        descriptor: &Descriptor,
        count: NonZeroU64,
    ) -> Result<DynamicVertexBuffer, Error> {
        Ok(DynamicVertexBuffer {
            inner: CDynamicBuffer::new(
                device,
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                false,
            )?,
        })
    }
}

impl<'s> Buffer<'s> for DynamicVertexBuffer<'s> {
    fn id(&self) -> u64 {
        self.inner.id()
    }
}

impl<'s> DynamicBuffer<'s> for DynamicVertexBuffer<'s> {
    fn as_slice<'a>(&self) -> Result<MappedSlice<'s, u8>, Error> {
        self.inner.as_slice()
    }
}

impl<'s> LayoutBuffer<'s> for DynamicVertexBuffer<'s> {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl<'s> VertexBufferLike<'s> for DynamicVertexBuffer<'s> {}
