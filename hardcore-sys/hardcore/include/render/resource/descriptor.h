#pragma once

#include "../../core/result.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief Primitive data types.
 */
enum HCPrimitive {
    HCPrimitive_Float, //!< A floating-point value.
    HCPrimitive_Integer, //!< A signed integer value.
    HCPrimitive_Unsigned, //!< An unsigned value.
    HCPrimitive_Boolean, //!< A boolean value.
};

/**
 * @brief Data type compositions.
 *
 * Matrices are described in a row-major layout (rows x columns).
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

struct HCBasicDescriptor {
    enum HCPrimitive primitive_type;
    uint32_t primitive_size; // In bits.
    enum HCComposition composition;
    uint32_t matrix_stride; // In bytes.
};

struct HCArrayDescriptor {
    uint16_t element_type_idx;
    uint32_t count;
    uint32_t stride;
};

struct HCPointerDescriptor {
    uint16_t type_idx;
};

struct HCMemberDescriptor {
    uint16_t type_idx;
    uint32_t offset;
};

struct HCStructDescriptor {
    uint16_t first_member_type_idx;
    uint16_t member_count;
};

enum HCOpaqueDescriptor {
    HCOpaqueDescriptor_Texture,
};

enum HCDescriptorCategory {
    HCDescriptorCategory_Basic,
    HCDescriptorCategory_Array,
    HCDescriptorCategory_Pointer,
    HCDescriptorCategory_Member,
    HCDescriptorCategory_Struct,
    HCDescriptorCategory_Opaque,
};

union HCGenericDescriptor {
    struct HCBasicDescriptor basic_descriptor;
    struct HCArrayDescriptor array_descriptor;
    struct HCPointerDescriptor pointer_descriptor;
    struct HCMemberDescriptor member_descriptor;
    struct HCStructDescriptor struct_descriptor;
    enum HCOpaqueDescriptor opaque_descriptor;
};

struct HCTypeDescriptor {
    enum HCDescriptorCategory type_category;
    union HCGenericDescriptor type_descriptor;
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
