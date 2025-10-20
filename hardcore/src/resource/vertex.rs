use crate::resource::buffer::{
    Buffer, BufferError, CBuffer, CDynamicBuffer, DynamicBuffer, LayoutBuffer, MappedSlice,
    ShaderWritableBuffer, VertexBufferLike,
};
use crate::resource::descriptor::Descriptor;

use std::num::NonZeroU64;

pub struct VertexBuffer<'s, const WRITABLE: bool> {
    inner: CBuffer<'s>,
}

impl<'s, const WRITABLE: bool> VertexBuffer<'s, WRITABLE> {
    pub(crate) fn create<'d>(
        device: u32,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
    ) -> Result<VertexBuffer<'s, WRITABLE>, BufferError>
    where
        's: 'd,
    {
        Ok(VertexBuffer::<'s, WRITABLE> {
            inner: CBuffer::create(
                device,
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                WRITABLE,
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
    pub(crate) fn create(
        device: u32,
        descriptor: &Descriptor,
        count: NonZeroU64,
    ) -> Result<DynamicVertexBuffer, BufferError> {
        Ok(DynamicVertexBuffer {
            inner: CDynamicBuffer::create(
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
    fn as_slice<'a>(&self) -> Result<MappedSlice<'s, u8>, BufferError> {
        self.inner.as_slice()
    }
}

impl<'s> LayoutBuffer<'s> for DynamicVertexBuffer<'s> {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl<'s> VertexBufferLike<'s> for DynamicVertexBuffer<'s> {}
