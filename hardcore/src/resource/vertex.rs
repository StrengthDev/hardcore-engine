use crate::context_token::ContextDependent;
use crate::resource::buffer::{
    Buffer, BufferError, CBuffer, ShaderWritableBuffer, VertexBufferLike,
};
use crate::resource::descriptor::Descriptor;

pub struct VertexBuffer<const WRITABLE: bool> {
    inner: CBuffer,
}

impl<const WRITABLE: bool> VertexBuffer<WRITABLE> {
    pub fn create(
        descriptor: &Descriptor,
        count: u64,
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

impl<const WRITABLE: bool> VertexBufferLike for VertexBuffer<WRITABLE> {}

impl ShaderWritableBuffer for VertexBuffer<true> {}
