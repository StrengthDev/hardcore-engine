#pragma once

#include "../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief The basic data type of a descriptor's field.
 */
enum HCPrimitive {
    HCPrimitive_U8, //!< An unsigned 8-bit integer.
    HCPrimitive_U16, //!< An unsigned 16-bit integer.
    HCPrimitive_U32, //!< An unsigned 32-bit integer.
    HCPrimitive_U64, //!< An unsigned 64-bit integer.
    HCPrimitive_I8, //!< An 8-bit integer.
    HCPrimitive_I16, //!< A 16-bit integer.
    HCPrimitive_I32, //!< A 32-bit integer.
    HCPrimitive_I64, //!< A 64-bit integer.
    HCPrimitive_F32, //!< A 32-bit floating point number.
    HCPrimitive_F64, //!< A 64-bit floating point number.
    HCPrimitive_B32, //!< A 32-bit boolean value.
};

/**
 * @brief The composition of a descriptor's field.
 *
 * ### Note on matrices
 *
 * The matrix layout used here matches the one used in GLSL, that is, a MatNxM matrix has N columns and M rows,
 * which is backwards from convention in mathematics.
 */
enum HCComposition {
    HCComposition_Scalar, //!< A singular value.
    HCComposition_Vec2, //!< A 2 element vector/array of values.
    HCComposition_Vec3, //!< A 3 element vector/array of values.
    HCComposition_Vec4, //!< A 4 element vector/array of values.
    HCComposition_Mat2x2, //!< A 2x2 matrix of values.
    HCComposition_Mat2x3, //!< A 2x3 matrix of values.
    HCComposition_Mat2x4, //!< A 2x4 matrix of values.
    HCComposition_Mat3x2, //!< A 3x2 matrix of values.
    HCComposition_Mat3x3, //!< A 3x3 matrix of values.
    HCComposition_Mat3x4, //!< A 3x4 matrix of values.
    HCComposition_Mat4x2, //!< A 4x2 matrix of values.
    HCComposition_Mat4x3, //!< A 4x3 matrix of values.
    HCComposition_Mat4x4, //!< A 4x4 matrix of values.
};

/**
 * @brief A description of a descriptor's field.
 */
struct HCField {
    enum HCPrimitive kind; //!< The basic data type of the field.
    enum HCComposition composition; //!< The composition of the field.
};

/**
 * @brief TODO
 */
enum HCAlignment {
    HCAlignment_Unknown,
    HCAlignment_Std140,
    HCAlignment_Std430,
};

/**
 * @brief A data descriptor.
 *
 * This struct should never be created directly. Instead, `hc_create_descriptor` should be used to create a new
 * instance, and `hc_destroy_descriptor` used to destroy the instance.
 */
struct HCDescriptor {
    struct HCField* fields;
    size_t field_count; //!< The number of fields. This value MUST be treated as const and never be changed.
    enum HCAlignment alignment;
};

/**
 * @brief Create a new `HCDescriptor`.
 *
 * All fields in the new descriptor are uninitialised and should be assigned correct values after creation.
 *
 * @param descriptor The pointer into which descriptor data will be written to.
 * @param field_count The number of fields in the descriptor.
 * @return The operation's result.
 */
struct HCResult hc_create_descriptor(struct HCDescriptor* descriptor, size_t field_count);

/**
 * @brief Destroys a `HCDescriptor`.
 *
 * @param descriptor A pointer to the `HCDescriptor` to be destroyed.
 */
void hc_destroy_descriptor(struct HCDescriptor* descriptor);

#ifdef __cplusplus
}
#endif // __cplusplus
