use crate::context_token::ContextDependent;
use crate::layer::Context;
use crate::resource::buffer::{
    Buffer, BufferError, CBuffer, CDynamicBuffer, DynamicBuffer, LayoutBuffer, MappedSlice,
    ShaderWritableBuffer, VertexBufferLike,
};
use crate::resource::descriptor::Descriptor;
use std::num::NonZeroU64;

pub struct VertexBuffer<const WRITABLE: bool> {
    inner: CBuffer,
}

impl<const WRITABLE: bool> VertexBuffer<WRITABLE> {
    pub fn create(
        descriptor: &Descriptor,
        count: NonZeroU64,
        device: u32,
    ) -> Result<VertexBuffer<WRITABLE>, BufferError> {
        Ok(VertexBuffer::<WRITABLE> {
            inner: CBuffer::create(
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                WRITABLE,
                device,
            )?,
        })
    }
}

impl<const WRITABLE: bool> ContextDependent for VertexBuffer<WRITABLE> {
    fn valid(&self) -> bool {
        self.inner.valid()
    }
}

impl<const WRITABLE: bool> Buffer for VertexBuffer<WRITABLE> {
    fn id(&self) -> u64 {
        self.inner.id()
    }
}

impl<const WRITABLE: bool> LayoutBuffer for VertexBuffer<WRITABLE> {
    fn layout(&self) -> &Descriptor {
        self.inner.layout()
    }
}

impl<const WRITABLE: bool> VertexBufferLike for VertexBuffer<WRITABLE> {}

impl ShaderWritableBuffer for VertexBuffer<true> {}

pub struct DynamicVertexBuffer {
    inner: CDynamicBuffer,
}

impl DynamicVertexBuffer {
    pub fn create(
        descriptor: &Descriptor,
        count: NonZeroU64,
        device: u32,
    ) -> Result<DynamicVertexBuffer, BufferError> {
        Ok(DynamicVertexBuffer {
            inner: CDynamicBuffer::create(
                hardcore_sys::BufferKind::Vertex,
                descriptor,
                count,
                false,
                device,
            )?,
        })
    }
}

impl ContextDependent for DynamicVertexBuffer {
    fn valid(&self) -> bool {
        self.inner.valid()
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
