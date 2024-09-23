use crate::context_token::{ContextDependent, ContextToken, ContextTokenError};
use crate::resource::descriptor::{CDescriptorError, Descriptor};
use hardcore_sys;
use std::ptr;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum BufferError {
    #[error(transparent)]
    Descriptor(#[from] CDescriptorError),
    #[error(transparent)]
    Context(#[from] ContextTokenError),
    #[error("Failed to create new buffer")]
    Initialisation,
}

pub(crate) trait Buffer: ContextDependent {
    fn id(&self) -> u64;
}

pub(crate) struct CBuffer {
    context_token: ContextToken,
    handle: hardcore_sys::Buffer,
}

impl CBuffer {
    pub(crate) fn create(
        kind: hardcore_sys::BufferKind,
        descriptor: &Descriptor,
        count: u64,
        writable: bool,
        device: u32,
    ) -> Result<Self, BufferError> {
        let token = ContextToken::new()?;

        let handle = unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_buffer(kind, c_desc.handle(), count, writable, device)
        };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CBuffer {
            context_token: token,
            handle,
        })
    }

    pub(crate) fn create_index(
        kind: hardcore_sys::Primitive,
        count: u64,
        writable: bool,
        device: u32,
    ) -> Result<Self, BufferError> {
        let token = ContextToken::new()?;

        let handle = unsafe { hardcore_sys::new_index_buffer(kind, count, writable, device) };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CBuffer {
            context_token: token,
            handle,
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.id
    }
}

impl Drop for CBuffer {
    fn drop(&mut self) {
        if self.context_token.valid() {
            unsafe { hardcore_sys::destroy_buffer(ptr::addr_of_mut!(self.handle)) }
        }
    }
}

impl ContextDependent for CBuffer {
    fn valid(&self) -> bool {
        self.context_token.valid()
    }
}

pub(crate) trait DynamicBuffer: Buffer {}

pub(crate) struct CDynamicBuffer {
    context_token: ContextToken,
    handle: hardcore_sys::DynamicBuffer,
}

impl CDynamicBuffer {
    pub(crate) fn create(
        kind: hardcore_sys::BufferKind,
        descriptor: Descriptor,
        count: u64,
        writable: bool,
        device: u32,
    ) -> Result<Self, BufferError> {
        let token = ContextToken::new()?;

        let handle = unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_dynamic_buffer(kind, c_desc.handle(), count, writable, device)
        };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CDynamicBuffer {
            context_token: token,
            handle,
        })
    }

    pub(crate) fn create_index(
        kind: hardcore_sys::Primitive,
        count: u64,
        writable: bool,
        device: u32,
    ) -> Result<Self, BufferError> {
        let token = ContextToken::new()?;

        let handle =
            unsafe { hardcore_sys::new_dynamic_index_buffer(kind, count, writable, device) };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CDynamicBuffer {
            context_token: token,
            handle,
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.id
    }

    pub(crate) fn host_ptr(&self) -> Result<*mut core::ffi::c_void, BufferError> {
        self.context_token.ok()?;

        let ptr = unsafe { self.handle.data.read().byte_add(self.handle.data_offset) };
        Ok(ptr)
    }
}

impl Drop for CDynamicBuffer {
    fn drop(&mut self) {
        if self.context_token.valid() {
            unsafe { hardcore_sys::destroy_dynamic_buffer(ptr::addr_of_mut!(self.handle)) }
        }
    }
}

pub(crate) trait ShaderReadableBuffer: Buffer {}

pub(crate) trait ShaderWritableBuffer: ShaderReadableBuffer {}

impl<T> ShaderReadableBuffer for T where T: ShaderWritableBuffer {}

pub(crate) trait VertexBufferLike: Buffer {}

pub(crate) trait IndexBufferLike: Buffer {}

pub(crate) trait UniformBufferLike: ShaderReadableBuffer {}

pub(crate) trait StorageBufferLike: ShaderWritableBuffer {}
