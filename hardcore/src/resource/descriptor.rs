use hardcore_sys;
use std::ptr;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum PrimitiveError {
    #[error("The value of the `hardcore_sys::Primitive` is invalid")]
    InvalidValue,
}

/// The basic datatype of a descriptor's field.
#[derive(Copy, Clone)]
pub enum Primitive {
    /// An unsigned 8-bit integer.
    U8,
    /// An unsigned 16-bit integer.
    U16,
    /// An unsigned 32-bit integer.
    U32,
    /// An unsigned 64-bit integer.
    U64,
    /// An 8-bit integer.
    I8,
    /// A 16-bit integer.
    I16,
    /// A 32-bit integer.
    I32,
    /// A 64-bit integer.
    I64,
    /// A 32-bit floating point number.
    F32,
    /// A 64-bit floating point number.
    F64,
    /// A 32-bit boolean value.
    B32,
}

impl Primitive {
    pub fn size(&self) -> usize {
        match self {
            Primitive::U8 | Primitive::I8 => 1,
            Primitive::U16 | Primitive::I16 => 2,
            Primitive::U32 | Primitive::I32 | Primitive::F32 | Primitive::B32 => 4,
            Primitive::U64 | Primitive::I64 | Primitive::F64 => 8,
        }
    }

    pub fn is_valid_index(&self) -> bool {
        matches!(self, Primitive::U8 | Primitive::U16 | Primitive::U32)
    }
}

impl TryFrom<hardcore_sys::Primitive> for Primitive {
    type Error = PrimitiveError;

    fn try_from(value: hardcore_sys::Primitive) -> Result<Self, Self::Error> {
        match value {
            hardcore_sys::Primitive::U8 => Ok(Primitive::U8),
            hardcore_sys::Primitive::U16 => Ok(Primitive::U16),
            hardcore_sys::Primitive::U32 => Ok(Primitive::U32),
            hardcore_sys::Primitive::U64 => Ok(Primitive::U64),
            hardcore_sys::Primitive::I8 => Ok(Primitive::I8),
            hardcore_sys::Primitive::I16 => Ok(Primitive::I16),
            hardcore_sys::Primitive::I32 => Ok(Primitive::I32),
            hardcore_sys::Primitive::I64 => Ok(Primitive::I64),
            hardcore_sys::Primitive::F32 => Ok(Primitive::F32),
            hardcore_sys::Primitive::F64 => Ok(Primitive::F64),
            hardcore_sys::Primitive::B32 => Ok(Primitive::B32),
            _ => Err(PrimitiveError::InvalidValue),
        }
    }
}

impl From<Primitive> for hardcore_sys::Primitive {
    fn from(value: Primitive) -> Self {
        match value {
            Primitive::U8 => hardcore_sys::Primitive::U8,
            Primitive::U16 => hardcore_sys::Primitive::U16,
            Primitive::U32 => hardcore_sys::Primitive::U32,
            Primitive::U64 => hardcore_sys::Primitive::U64,
            Primitive::I8 => hardcore_sys::Primitive::I8,
            Primitive::I16 => hardcore_sys::Primitive::I16,
            Primitive::I32 => hardcore_sys::Primitive::I32,
            Primitive::I64 => hardcore_sys::Primitive::I64,
            Primitive::F32 => hardcore_sys::Primitive::F32,
            Primitive::F64 => hardcore_sys::Primitive::F64,
            Primitive::B32 => hardcore_sys::Primitive::B32,
        }
    }
}

#[derive(Error, Debug)]
pub enum CompositionError {
    #[error("The value of the `hardcore_sys::Composition` is invalid")]
    InvalidValue,
}

/// The composition of a descriptor's field.
///
/// ### Note on matrices
///
/// The matrix layout used here matches the one used in GLSL, that is, a MatNxM matrix has N
/// columns and M rows, which is backward from convention in mathematics.
#[derive(Copy, Clone)]
pub enum Composition {
    /// A singular value.
    Scalar,
    /// A 2 element vector/array of values.
    Vec2,
    /// A 3 element vector/array of values.
    Vec3,
    /// A 4 element vector/array of values.
    Vec4,
    /// A 2x2 matrix of values.
    Mat2x2,
    /// A 2x3 matrix of values.
    Mat2x3,
    /// A 2x4 matrix of values.
    Mat2x4,
    /// A 3x2 matrix of values.
    Mat3x2,
    /// A 3x3 matrix of values.
    Mat3x3,
    /// A 3x4 matrix of values.
    Mat3x4,
    /// A 4x2 matrix of values.
    Mat4x2,
    /// A 4x3 matrix of values.
    Mat4x3,
    /// A 4x4 matrix of values.
    Mat4x4,
}

impl Composition {
    pub fn count(&self) -> usize {
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
        }
    }
}

impl TryFrom<hardcore_sys::Composition> for Composition {
    type Error = CompositionError;

    fn try_from(value: hardcore_sys::Composition) -> Result<Self, Self::Error> {
        match value {
            hardcore_sys::Composition::Scalar => Ok(Composition::Scalar),
            hardcore_sys::Composition::Vec2 => Ok(Composition::Vec2),
            hardcore_sys::Composition::Vec3 => Ok(Composition::Vec3),
            hardcore_sys::Composition::Vec4 => Ok(Composition::Vec4),
            hardcore_sys::Composition::Mat2x2 => Ok(Composition::Mat2x2),
            hardcore_sys::Composition::Mat2x3 => Ok(Composition::Mat2x3),
            hardcore_sys::Composition::Mat2x4 => Ok(Composition::Mat2x4),
            hardcore_sys::Composition::Mat3x2 => Ok(Composition::Mat3x2),
            hardcore_sys::Composition::Mat3x3 => Ok(Composition::Mat3x3),
            hardcore_sys::Composition::Mat3x4 => Ok(Composition::Mat3x4),
            hardcore_sys::Composition::Mat4x2 => Ok(Composition::Mat4x2),
            hardcore_sys::Composition::Mat4x3 => Ok(Composition::Mat4x3),
            hardcore_sys::Composition::Mat4x4 => Ok(Composition::Mat4x4),
            _ => Err(CompositionError::InvalidValue),
        }
    }
}

impl From<Composition> for hardcore_sys::Composition {
    fn from(value: Composition) -> Self {
        match value {
            Composition::Scalar => hardcore_sys::Composition::Scalar,
            Composition::Vec2 => hardcore_sys::Composition::Vec2,
            Composition::Vec3 => hardcore_sys::Composition::Vec3,
            Composition::Vec4 => hardcore_sys::Composition::Vec4,
            Composition::Mat2x2 => hardcore_sys::Composition::Mat2x2,
            Composition::Mat2x3 => hardcore_sys::Composition::Mat2x3,
            Composition::Mat2x4 => hardcore_sys::Composition::Mat2x4,
            Composition::Mat3x2 => hardcore_sys::Composition::Mat3x2,
            Composition::Mat3x3 => hardcore_sys::Composition::Mat3x3,
            Composition::Mat3x4 => hardcore_sys::Composition::Mat3x4,
            Composition::Mat4x2 => hardcore_sys::Composition::Mat4x2,
            Composition::Mat4x3 => hardcore_sys::Composition::Mat4x3,
            Composition::Mat4x4 => hardcore_sys::Composition::Mat4x4,
        }
    }
}

#[derive(Error, Debug)]
pub enum FieldError {
    #[error(transparent)]
    InvalidPrimitiveValue(#[from] PrimitiveError),
    #[error(transparent)]
    InvalidCompositionValue(#[from] CompositionError),
}

/// A description of a descriptor's field.
#[derive(Copy, Clone)]
pub struct Field {
    /// The basic data type of the field.
    pub kind: Primitive,
    /// The composition of the field.
    pub composition: Composition,
}

impl Field {
    pub fn size(&self) -> usize {
        self.kind.size() * self.composition.count()
    }
}

impl TryFrom<hardcore_sys::Field> for Field {
    type Error = FieldError;

    fn try_from(value: hardcore_sys::Field) -> Result<Self, Self::Error> {
        Ok(Self {
            kind: value.kind.try_into()?,
            composition: value.composition.try_into()?,
        })
    }
}

impl From<Field> for hardcore_sys::Field {
    fn from(value: Field) -> Self {
        Self {
            kind: value.kind.into(),
            composition: value.composition.into(),
        }
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
    #[error("Failed to create native descriptor")]
    CreationError,
}

pub(crate) struct CDescriptor {
    inner: hardcore_sys::Descriptor,
}

impl CDescriptor {
    // TODO rename constructor type functions to new
    fn create(field_count: usize) -> Result<Self, CDescriptorError> {
        let desc = unsafe { hardcore_sys::create_descriptor(field_count) };

        if desc.fields.is_null() {
            return Err(CDescriptorError::CreationError);
        }

        Ok(CDescriptor { inner: desc })
    }

    pub(crate) fn handle(&self) -> *const hardcore_sys::Descriptor {
        ptr::addr_of!(self.inner)
    }

    fn as_slice(&self) -> &[hardcore_sys::Field] {
        unsafe { std::slice::from_raw_parts(self.inner.fields, self.inner.field_count) }
    }

    fn as_mut_slice(&mut self) -> &mut [hardcore_sys::Field] {
        unsafe { std::slice::from_raw_parts_mut(self.inner.fields, self.inner.field_count) }
    }

    fn try_desc(&self) -> Result<Descriptor, FieldError> {
        let mut desc = Descriptor {
            fields: Vec::with_capacity(self.inner.field_count),
            alignment: Alignment::Unknown,
        };

        for c_field in self.as_slice() {
            desc.fields.push((*c_field).try_into()?);
        }

        Ok(desc)
    }
}

impl Drop for CDescriptor {
    fn drop(&mut self) {
        unsafe {
            hardcore_sys::destroy_descriptor(ptr::addr_of_mut!(self.inner));
        }
    }
}
