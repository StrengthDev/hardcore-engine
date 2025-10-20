use crate::resource::descriptor::{CDescriptorError, Descriptor, PrimitiveExt};

use hardcore_sys;
use hardcore_sys::Primitive;

use crate::dependent_handle::DependentHandle;
use std::marker::PhantomData;
use std::num::NonZeroU64;
use std::ops::{Deref, DerefMut};
use std::ptr;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum BufferError {
    /// An error has occurred withing the system crate.
    #[error(transparent)]
    SystemError(#[from] hardcore_sys::Error),

    #[error(transparent)]
    Descriptor(#[from] CDescriptorError),

    #[error("Invalid index type")]
    Index,
}

pub trait Buffer<'s> {
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

pub(crate) struct CBuffer<'s> {
    content_kind: BufferContentKind,
    handle: DependentHandle<'s, hardcore_sys::Buffer>,
}

impl<'s> CBuffer<'s> {
    pub(crate) fn create<'d>(
        device: u32,
        kind: hardcore_sys::BufferKind,
        descriptor: &'d Descriptor,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError>
    where
        's: 'd,
    {
        let mut handle = Default::default();
        unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_buffer(
                &raw mut handle,
                device,
                kind,
                c_desc.ptr(),
                count.into(),
                writable,
            )
            .into_std_result()?
        };

        Ok(CBuffer {
            content_kind: BufferContentKind::Layout(descriptor.clone()),
            handle: handle.into(),
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

        let mut handle = Default::default();
        unsafe {
            hardcore_sys::new_index_buffer(
                &raw mut handle,
                device,
                kind.into(),
                count.into(),
                writable,
            )
            .into_std_result()?
        };

        Ok(CBuffer {
            content_kind: BufferContentKind::Index(kind),
            handle: handle.into(),
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.inner.id
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

impl<'s> Drop for CBuffer<'s> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_buffer(self.handle.mut_ptr()) }
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

pub trait DynamicBuffer<'s>: Buffer<'s> {
    fn as_slice(&self) -> Result<MappedSlice<'s, u8>, BufferError>;
}

pub(crate) struct CDynamicBuffer<'s> {
    content_kind: BufferContentKind,
    handle: DependentHandle<'s, hardcore_sys::DynamicBuffer>,
}

impl<'s> CDynamicBuffer<'s> {
    pub(crate) fn create(
        device: u32,
        kind: hardcore_sys::BufferKind,
        descriptor: &Descriptor,
        count: NonZeroU64,
        writable: bool,
    ) -> Result<Self, BufferError> {
        let mut handle = Default::default();
        unsafe {
            let c_desc = descriptor.c_desc()?;
            hardcore_sys::new_dynamic_buffer(
                &raw mut handle,
                device,
                kind,
                c_desc.ptr(),
                count.into(),
                writable,
            )
            .into_std_result()?
        };

        Ok(CDynamicBuffer {
            content_kind: BufferContentKind::Layout(descriptor.clone()),
            handle: handle.into(),
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

        let mut handle = Default::default();
        unsafe {
            hardcore_sys::new_dynamic_index_buffer(
                &raw mut handle,
                device,
                kind.into(),
                count.into(),
                writable,
            )
            .into_std_result()?
        };

        Ok(CDynamicBuffer {
            content_kind: BufferContentKind::Index(kind),
            handle: handle.into(),
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.inner.id
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
        let ptr = unsafe {
            self.handle
                .inner
                .data
                .read()
                .byte_add(self.handle.inner.data_offset)
        };
        Ok(ptr.cast())
    }

    pub(crate) fn as_slice(&self) -> Result<MappedSlice<'s, u8>, BufferError> {
        Ok(MappedSlice {
            ptr: ptr::slice_from_raw_parts_mut(self.host_ptr()?, self.content_kind.size()),
            phantom_data: PhantomData,
        })
    }
}

impl<'s> Drop for CDynamicBuffer<'s> {
    fn drop(&mut self) {
        unsafe { hardcore_sys::destroy_dynamic_buffer(self.handle.mut_ptr()) }
    }
}

unsafe impl<'s> Send for CDynamicBuffer<'s> {}

pub trait ShaderReadableBuffer<'s>: Buffer<'s> {}

pub trait ShaderWritableBuffer<'s>: ShaderReadableBuffer<'s> {}

impl<'s, T> ShaderReadableBuffer<'s> for T where T: ShaderWritableBuffer<'s> {}

pub trait LayoutBuffer<'s>: Buffer<'s> {
    fn layout(&self) -> &Descriptor;
}

pub trait VertexBufferLike<'s>: LayoutBuffer<'s> {}

pub trait IndexBufferLike<'s>: Buffer<'s> {
    fn index_kind(&self) -> &Primitive;
}

pub trait UniformBufferLike<'s>: LayoutBuffer<'s> + ShaderReadableBuffer<'s> {}

pub trait StorageBufferLike<'s>: LayoutBuffer<'s> + ShaderWritableBuffer<'s> {}
