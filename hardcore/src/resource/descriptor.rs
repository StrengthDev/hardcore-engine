use crate::Error;

use hardcore_sys::{DescriptorCategory, Field, TypeDescriptor};

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

#[derive(Debug)]
pub enum Descriptor2 {
    Basic {
        primitive_type: Primitive,
        primitive_size: u32,
        composition: Composition,
        matrix_stride: u32,
    },
    Array {
        element_descriptor: Box<Descriptor2>,
        count: u32,
        stride: u32,
    },
    Pointer(Box<Descriptor2>),
    Struct {
        members: Vec<(u32, Descriptor2)>,
    },
    OpaqueType,
}

fn parse_descriptor(descriptor_data: &[TypeDescriptor], index: u16) -> Result<Descriptor2, Error> {
    let descriptor = &descriptor_data[index as usize];

    match descriptor.type_category {
        DescriptorCategory::Basic => {
            let descriptor = unsafe { &descriptor.type_descriptor.basic_descriptor };

            Ok(Descriptor2::Basic {
                primitive_type: descriptor.primitive_type,
                primitive_size: descriptor.primitive_size,
                composition: descriptor.composition,
                matrix_stride: descriptor.matrix_stride,
            })
        }
        DescriptorCategory::Array => {
            let descriptor = unsafe { &descriptor.type_descriptor.array_descriptor };

            Ok(Descriptor2::Array {
                element_descriptor: Box::new(parse_descriptor(
                    descriptor_data,
                    descriptor.element_type_idx,
                )?),
                count: descriptor.count,
                stride: descriptor.stride,
            })
        }
        DescriptorCategory::Pointer => {
            let descriptor = unsafe { &descriptor.type_descriptor.pointer_descriptor };

            Ok(Descriptor2::Pointer(Box::new(parse_descriptor(
                descriptor_data,
                descriptor.type_idx,
            )?)))
        }
        DescriptorCategory::Struct => {
            let descriptor = unsafe { &descriptor.type_descriptor.struct_descriptor };

            let mut members: Vec<(u32, Descriptor2)> =
                Vec::with_capacity(descriptor.member_count as usize);

            for i in descriptor.first_member_type_idx
                ..descriptor.first_member_type_idx + descriptor.member_count
            {
                let member = &descriptor_data[i as usize];
                if member.type_category != DescriptorCategory::Member {
                    return Err(Error::UnexpectedValue);
                }

                let member = unsafe { &member.type_descriptor.member_descriptor };
                members.push((
                    member.offset,
                    parse_descriptor(descriptor_data, member.type_idx)?,
                ));
            }

            Ok(Descriptor2::Struct { members })
        }
        DescriptorCategory::Opaque => Ok(Descriptor2::OpaqueType),
        _ => Err(Error::UnexpectedValue),
    }
}

impl TryFrom<&[TypeDescriptor]> for Descriptor2 {
    type Error = Error;

    fn try_from(value: &[TypeDescriptor]) -> Result<Self, Self::Error> {
        parse_descriptor(value, 0)
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

    pub(crate) fn c_desc(&self) -> Result<CDescriptor, Error> {
        let mut c_desc = CDescriptor::new(self.fields.len())?;

        for (field, c_field) in self.fields.iter().zip(c_desc.as_mut_slice()) {
            *c_field = (*field).into();
        }

        Ok(c_desc)
    }
}

pub(crate) struct CDescriptor {
    inner: hardcore_sys::Descriptor,
}

impl CDescriptor {
    fn new(field_count: usize) -> Result<CDescriptor, Error> {
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
