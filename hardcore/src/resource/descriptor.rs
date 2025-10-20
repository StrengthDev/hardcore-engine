use hardcore_sys::Field;
use thiserror::Error;

pub use hardcore_sys::{Composition, Primitive};

#[forbid(missing_docs)]
mod seal {
    pub trait Seal {}
}

pub trait PrimitiveExt: seal::Seal {
    fn size(&self) -> usize;
    fn is_valid_index(&self) -> bool;
}

impl seal::Seal for Primitive {}

impl PrimitiveExt for Primitive {
    fn size(&self) -> usize {
        // match self {
        //     Primitive::U8 | Primitive::I8 => 1,
        //     Primitive::U16 | Primitive::I16 => 2,
        //     Primitive::U32 | Primitive::I32 | Primitive::F32 | Primitive::B32 => 4,
        //     Primitive::U64 | Primitive::I64 | Primitive::F64 => 8,
        //     _ => 0,
        // }
        0
    }

    fn is_valid_index(&self) -> bool {
        *self == Primitive::Unsigned
    }
}

pub trait CompositionExt: seal::Seal {
    fn count(&self) -> usize;
}

impl seal::Seal for Composition {}

impl CompositionExt for Composition {
    fn count(&self) -> usize {
        match self {
            Composition::Scalar => 1,
            Composition::Vec2 => 2,
            Composition::Vec3 => 3,
            Composition::Vec4 | Composition::Mat2x2 => 4,
            Composition::Mat2x3 | Composition::Mat3x2 => 6,
            Composition::Mat2x4 | Composition::Mat4x2 => 8,
            Composition::Mat4x3 | Composition::Mat3x4 => 12,
            Composition::Mat3x3 => 9,
            Composition::Mat4x4 => 16,
            _ => 0,
        }
    }
}

pub trait FieldExt: seal::Seal {
    fn size(&self) -> usize;
}

impl seal::Seal for Field {}

impl FieldExt for Field {
    fn size(&self) -> usize {
        self.kind.size() * self.composition.count()
    }
}

#[derive(Copy, Clone, Default)]
enum Alignment {
    Std140,
    Std430,
    #[default]
    Unknown,
}

#[derive(Clone, Default)]
pub struct Descriptor {
    fields: Vec<Field>,
    alignment: Alignment,
}

impl Descriptor {
    pub fn push(&mut self, p: Primitive, c: Composition) {
        self.fields.push(Field {
            kind: p,
            composition: c,
        })
    }

    pub fn push_field(&mut self, field: Field) {
        self.fields.push(field)
    }

    // TODO this may need to be revised based on the alignment https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
    pub fn size(&self) -> usize {
        self.fields
            .iter()
            .map(|f| f.size())
            .reduce(|f0, f1| f0 + f1)
            .unwrap_or(0)
    }

    pub(crate) fn c_desc(&self) -> Result<CDescriptor, CDescriptorError> {
        let mut c_desc = CDescriptor::create(self.fields.len())?;

        for (field, c_field) in self.fields.iter().zip(c_desc.as_mut_slice()) {
            *c_field = (*field).into();
        }

        Ok(c_desc)
    }
}

#[derive(Error, Debug)]
pub enum CDescriptorError {
    /// An error has occurred withing the system crate.
    #[error(transparent)]
    SystemError(#[from] hardcore_sys::Error),
}

pub(crate) struct CDescriptor {
    inner: hardcore_sys::Descriptor,
}

impl CDescriptor {
    // TODO rename constructor type functions to new
    fn create(field_count: usize) -> Result<Self, CDescriptorError> {
        let mut descriptor = CDescriptor {
            inner: Default::default(),
        };

        unsafe {
            hardcore_sys::create_descriptor(&raw mut descriptor.inner, field_count)
                .into_std_result()?
        };

        Ok(descriptor)
    }

    pub(crate) fn ptr(&self) -> *const hardcore_sys::Descriptor {
        &raw const self.inner
    }

    fn as_slice(&self) -> &[Field] {
        unsafe { std::slice::from_raw_parts(self.inner.fields, self.inner.field_count) }
    }

    fn as_mut_slice(&mut self) -> &mut [Field] {
        unsafe { std::slice::from_raw_parts_mut(self.inner.fields, self.inner.field_count) }
    }

    fn try_desc(&self) -> Descriptor {
        let mut desc = Descriptor {
            fields: Vec::with_capacity(self.inner.field_count),
            alignment: Alignment::Unknown,
        };

        for field in self.as_slice() {
            desc.fields.push(*field);
        }

        desc
    }
}

impl Drop for CDescriptor {
    fn drop(&mut self) {
        unsafe {
            hardcore_sys::destroy_descriptor(&raw mut self.inner);
        }
    }
}
