#[macro_export]
macro_rules! field {
    (bool) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::Bool,
            composition: hardcore_sys::Composition::Scalar,
        }
    };
    (int) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive,
            composition: hardcore_sys::Composition::Scalar,
        }
    };
    (uint) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::U32,
            composition: hardcore_sys::Composition::Scalar,
        }
    };
    (float) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Scalar,
        }
    };
    (double) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Scalar,
        }
    };
    (bvec2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::Bool,
            composition: hardcore_sys::Composition::Vec2,
        }
    };
    (bvec3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::Bool,
            composition: hardcore_sys::Composition::Vec3,
        }
    };
    (bvec4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::Bool,
            composition: hardcore_sys::Composition::Vec4,
        }
    };
    (ivec2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive,
            composition: hardcore_sys::Composition::Vec2,
        }
    };
    (ivec3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive,
            composition: hardcore_sys::Composition::Vec3,
        }
    };
    (ivec4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive,
            composition: hardcore_sys::Composition::Vec4,
        }
    };
    (uvec2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::U32,
            composition: hardcore_sys::Composition::Vec2,
        }
    };
    (uvec3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::U32,
            composition: hardcore_sys::Composition::Vec3,
        }
    };
    (uvec4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::U32,
            composition: hardcore_sys::Composition::Vec4,
        }
    };
    (vec2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Vec2,
        }
    };
    (vec3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Vec3,
        }
    };
    (vec4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Vec4,
        }
    };
    (dvec2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Vec2,
        }
    };
    (dvec3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Vec3,
        }
    };
    (dvec4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Vec4,
        }
    };
    (mat2x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat2x2,
        }
    };
    (mat2x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat2x3,
        }
    };
    (mat2x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat2x4,
        }
    };
    (mat2) => {
        $crate::field!(mat2x2)
    };
    (mat3x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat3x2,
        }
    };
    (mat3x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat3x3,
        }
    };
    (mat3x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat3x4,
        }
    };
    (mat3) => {
        $crate::field!(mat3x3)
    };
    (mat4x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat4x2,
        }
    };
    (mat4x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat4x3,
        }
    };
    (mat4x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F32,
            composition: hardcore_sys::Composition::Mat4x4,
        }
    };
    (mat4) => {
        $crate::field!(mat4x4)
    };
    (dmat2x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat2x2,
        }
    };
    (dmat2x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat2x3,
        }
    };
    (dmat2x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat2x4,
        }
    };
    (dmat2) => {
        $crate::field!(dmat2x2)
    };
    (dmat3x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat3x2,
        }
    };
    (dmat3x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat3x3,
        }
    };
    (dmat3x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat3x4,
        }
    };
    (dmat3) => {
        $crate::field!(dmat3x3)
    };
    (dmat4x2) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat4x2,
        }
    };
    (dmat4x3) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat4x3,
        }
    };
    (dmat4x4) => {
        hardcore_sys::Field {
            kind: hardcore_sys::Primitive::F64,
            composition: hardcore_sys::Composition::Mat4x4,
        }
    };
    (dmat4) => {
        $crate::field!(dmat4x4)
    };
}

#[macro_export]
macro_rules! descriptor {
    [$($field:ident),*] => {
        {
            let mut desc = $crate::resource::descriptor::Descriptor::default();
            $(desc.push_field($crate::field!($field));)*
            desc
        }
    };
}
