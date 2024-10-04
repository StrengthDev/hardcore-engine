#[macro_export]
macro_rules! field {
    (bool) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::Bool,
            composition: $crate::resource::descriptor::Composition::Scalar,
        }
    };
    (int) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::I32,
            composition: $crate::resource::descriptor::Composition::Scalar,
        }
    };
    (uint) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::U32,
            composition: $crate::resource::descriptor::Composition::Scalar,
        }
    };
    (float) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Scalar,
        }
    };
    (double) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Scalar,
        }
    };
    (bvec2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::Bool,
            composition: $crate::resource::descriptor::Composition::Vec2,
        }
    };
    (bvec3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::Bool,
            composition: $crate::resource::descriptor::Composition::Vec3,
        }
    };
    (bvec4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::Bool,
            composition: $crate::resource::descriptor::Composition::Vec4,
        }
    };
    (ivec2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::I32,
            composition: $crate::resource::descriptor::Composition::Vec2,
        }
    };
    (ivec3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::I32,
            composition: $crate::resource::descriptor::Composition::Vec3,
        }
    };
    (ivec4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::I32,
            composition: $crate::resource::descriptor::Composition::Vec4,
        }
    };
    (uvec2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::U32,
            composition: $crate::resource::descriptor::Composition::Vec2,
        }
    };
    (uvec3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::U32,
            composition: $crate::resource::descriptor::Composition::Vec3,
        }
    };
    (uvec4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::U32,
            composition: $crate::resource::descriptor::Composition::Vec4,
        }
    };
    (vec2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Vec2,
        }
    };
    (vec3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Vec3,
        }
    };
    (vec4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Vec4,
        }
    };
    (dvec2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Vec2,
        }
    };
    (dvec3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Vec3,
        }
    };
    (dvec4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Vec4,
        }
    };
    (mat2x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat2x2,
        }
    };
    (mat2x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat2x3,
        }
    };
    (mat2x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat2x4,
        }
    };
    (mat2) => {
        $crate::field!(mat2x2)
    };
    (mat3x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat3x2,
        }
    };
    (mat3x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat3x3,
        }
    };
    (mat3x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat3x4,
        }
    };
    (mat3) => {
        $crate::field!(mat3x3)
    };
    (mat4x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat4x2,
        }
    };
    (mat4x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat4x3,
        }
    };
    (mat4x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F32,
            composition: $crate::resource::descriptor::Composition::Mat4x4,
        }
    };
    (mat4) => {
        $crate::field!(mat4x4)
    };
    (dmat2x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat2x2,
        }
    };
    (dmat2x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat2x3,
        }
    };
    (dmat2x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat2x4,
        }
    };
    (dmat2) => {
        $crate::field!(dmat2x2)
    };
    (dmat3x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat3x2,
        }
    };
    (dmat3x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat3x3,
        }
    };
    (dmat3x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat3x4,
        }
    };
    (dmat3) => {
        $crate::field!(dmat3x3)
    };
    (dmat4x2) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat4x2,
        }
    };
    (dmat4x3) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat4x3,
        }
    };
    (dmat4x4) => {
        $crate::resource::descriptor::Field {
            kind: $crate::resource::descriptor::Primitive::F64,
            composition: $crate::resource::descriptor::Composition::Mat4x4,
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
