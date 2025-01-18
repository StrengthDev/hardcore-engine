use crate::context::Context;
use crate::resource::buffer::{
    Buffer, BufferError, CBuffer, CDynamicBuffer, DynamicBuffer, LayoutBuffer, MappedSlice,
    ShaderWritableBuffer, VertexBufferLike,
};
use crate::resource::descriptor::Descriptor;
use std::num::NonZeroU64;

pub struct VertexBuffer<'c, const WRITABLE: bool> {
    inner: CBuffer<'c>,
}

impl<'c, const WRITABLE: bool> VertexBuffer<'c, WRITABLE> {
    pub(crate) fn create<'d>(
        device: u32,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
    ) -> Result<VertexBuffer<'c, WRITABLE>, BufferError>
    where
        'c: 'd,
    {
        Ok(VertexBuffer::<'c, WRITABLE> {
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

impl<'c, const WRITABLE: bool> Buffer for VertexBuffer<'c, WRITABLE> {
    fn id(&self) -> u64 {
        self.inner.id()
    }
}

impl<'c, const WRITABLE: bool> LayoutBuffer for VertexBuffer<'c, WRITABLE> {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl<'c, const WRITABLE: bool> VertexBufferLike for VertexBuffer<'c, WRITABLE> {}

impl<'c> ShaderWritableBuffer for VertexBuffer<'c, true> {}

pub struct DynamicVertexBuffer {
    inner: CDynamicBuffer,
}

impl DynamicVertexBuffer {
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

impl Buffer for DynamicVertexBuffer {
    fn id(&self) -> u64 {
        self.inner.id()
    }
}

impl DynamicBuffer for DynamicVertexBuffer {
    fn as_slice<'a>(&self, context: &'a Context) -> Result<MappedSlice<'a, u8>, BufferError> {
        self.inner.as_slice(context)
    }
}

impl LayoutBuffer for DynamicVertexBuffer {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl VertexBufferLike for DynamicVertexBuffer {}
