use crate::context::Context;
use crate::resource::descriptor::{CDescriptorError, Descriptor, PrimitiveExt};
use hardcore_sys;
use hardcore_sys::Primitive;
use std::marker::PhantomData;
use std::num::NonZeroU64;
use std::ops::{Deref, DerefMut};
use std::ptr;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum BufferError {
    #[error(transparent)]
    Descriptor(#[from] CDescriptorError),

    #[error("Invalid index type")]
    Index,

    #[error("Failed to create new buffer")]
    Initialisation,
}

pub trait Buffer {
    fn id(&self) -> u64;
}

enum BufferContentKind {
    Layout(Descriptor),
    Index(Primitive),
}

impl BufferContentKind {
    fn size(&self) -> usize {
        match self {
            BufferContentKind::Layout(desc) => desc.size(),
            BufferContentKind::Index(prim) => prim.size(),
        }
    }
}

pub(crate) struct CBuffer<'c> {
    content_kind: BufferContentKind,
    handle: hardcore_sys::Buffer,
    context: PhantomData<&'c hardcore_sys::Buffer>,
}

impl<'c> CBuffer<'c> {
    pub(crate) fn create<'d>(
        device: u32,
        kind: hardcore_sys::BufferKind,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError>
    where
        'c: 'd,
    {
        let handle = unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_buffer(device, kind, c_desc.handle(), count.into(), writable)
        };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CBuffer {
            content_kind: BufferContentKind::Layout(descriptor.clone()),
            handle,
            context: PhantomData,
        })
    }

    pub(crate) fn create_index(
        device: u32,
        kind: Primitive,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError> {
        if !kind.is_valid_index() {
            return Err(BufferError::Index);
        }

        let handle =
            unsafe { hardcore_sys::new_index_buffer(device, kind.into(), count.into(), writable) };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CBuffer {
            content_kind: BufferContentKind::Index(kind),
            handle,
            context: PhantomData,
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.id
    }

    pub(crate) fn layout(&self) -> &Descriptor {
        if let BufferContentKind::Layout(ref desc) = self.content_kind {
            desc
        } else {
            unreachable!(
                "This function should only be called by buffers created using a descriptor"
            )
        }
    }

    pub(crate) fn index_kind(&self) -> &Primitive {
        if let BufferContentKind::Index(ref prim) = self.content_kind {
            prim
        } else {
            unreachable!("This function should only be called by index buffers")
        }
    }
}

impl<'c> Drop for CBuffer<'c> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_buffer(ptr::addr_of_mut!(self.handle)) }
    }
}

pub struct MappedSlice<'a, T> {
    ptr: *mut [T],
    phantom_data: PhantomData<&'a T>,
}

impl<'a, T> Deref for MappedSlice<'a, T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.ptr }
    }
}

impl<'a, T> DerefMut for MappedSlice<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.ptr }
    }
}

unsafe impl<'a, T> Send for MappedSlice<'a, T> {}

pub trait DynamicBuffer: Buffer {
    fn as_slice<'a>(&self, context: &'a Context) -> Result<MappedSlice<'a, u8>, BufferError>;
}

pub(crate) struct CDynamicBuffer {
    content_kind: BufferContentKind,
    handle: hardcore_sys::DynamicBuffer,
}

impl CDynamicBuffer {
    pub(crate) fn create(
        device: u32,
        kind: hardcore_sys::BufferKind,
        descriptor: &Descriptor,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError> {
        let handle = unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_dynamic_buffer(device, kind, c_desc.handle(), count.into(), writable)
        };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CDynamicBuffer {
            content_kind: BufferContentKind::Layout(descriptor.clone()),
            handle,
        })
    }

    pub(crate) fn create_index(
        device: u32,
        kind: Primitive,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError> {
        if !kind.is_valid_index() {
            return Err(BufferError::Index);
        }

        let handle = unsafe {
            hardcore_sys::new_dynamic_index_buffer(device, kind.into(), count.into(), writable)
        };

        if handle.size == 0 {
            return Err(BufferError::Initialisation);
        }

        Ok(CDynamicBuffer {
            content_kind: BufferContentKind::Index(kind),
            handle,
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.id
    }

    pub(crate) fn layout(&self) -> &Descriptor {
        if let BufferContentKind::Layout(ref desc) = self.content_kind {
            desc
        } else {
            unreachable!(
                "This function should only be called by buffers created using a descriptor"
            )
        }
    }

    pub(crate) fn index_kind(&self) -> &Primitive {
        if let BufferContentKind::Index(ref prim) = self.content_kind {
            prim
        } else {
            unreachable!("This function should only be called by index buffers")
        }
    }

    fn host_ptr(&self) -> Result<*mut u8, BufferError> {
        let ptr = unsafe { self.handle.data.read().byte_add(self.handle.data_offset) };
        Ok(ptr.cast())
    }

    pub(crate) fn as_slice<'a>(&self, _: &'a Context) -> Result<MappedSlice<'a, u8>, BufferError> {
        Ok(MappedSlice {
            ptr: ptr::slice_from_raw_parts_mut(self.host_ptr()?, self.content_kind.size()),
            phantom_data: PhantomData,
        })
    }
}

impl Drop for CDynamicBuffer {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_dynamic_buffer(ptr::addr_of_mut!(self.handle)) }
    }
}

unsafe impl Send for CDynamicBuffer {}

pub trait ShaderReadableBuffer: Buffer {}

pub trait ShaderWritableBuffer: ShaderReadableBuffer {}

impl<T> ShaderReadableBuffer for T where T: ShaderWritableBuffer {}

pub trait LayoutBuffer: Buffer {
    fn layout(&self) -> &Descriptor;
}

pub trait VertexBufferLike: LayoutBuffer {}

pub trait IndexBufferLike: Buffer {
    fn index_kind(&self) -> &Primitive;
}

pub trait UniformBufferLike: LayoutBuffer + ShaderReadableBuffer {}

pub trait StorageBufferLike: LayoutBuffer + ShaderWritableBuffer {}
