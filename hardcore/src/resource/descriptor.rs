/// The basic data type of a descriptor's field.
enum Primitive {
    U8, /// An unsigned 8-bit integer.
    U16, /// An unsigned 16-bit integer.
    U32, /// An unsigned 32-bit integer.
    U64, /// An unsigned 64-bit integer.
    I8, /// An 8-bit integer.
    I16, /// A 16-bit integer.
    I32, /// A 32-bit integer.
    I64, /// A 64-bit integer.
    F32, /// A 32-bit floating point number.
    F64, /// A 64-bit floating point number.
}

/// The composition of a descriptor's field.
enum Composition {
    Scalar, /// A singular value.
    Vec2, /// A 2 element vector/array of values.
    Vec3, /// A 3 element vector/array of values.
    Vec4, /// A 4 element vector/array of values.
}

/// A description of a descriptor's field.
struct Field {
    kind: Primitive, /// The basic data type of the field.
    composition: Composition, /// The composition of the field.
}

enum Alignment {
    Std140,
    Std430,
}

struct Descriptor {
    fields: Vec<Field>,
    alignment: Alignment,
}
