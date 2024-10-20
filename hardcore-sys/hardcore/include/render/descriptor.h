#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief The basic data type of a descriptor's field.
 */
enum HCPrimitive {
	U8, //!< An unsigned 8-bit integer.
	U16, //!< An unsigned 16-bit integer.
	U32, //!< An unsigned 32-bit integer.
	U64, //!< An unsigned 64-bit integer.
	I8, //!< An 8-bit integer.
	I16, //!< A 16-bit integer.
	I32, //!< A 32-bit integer.
	I64, //!< A 64-bit integer.
	F32, //!< A 32-bit floating point number.
	F64, //!< A 64-bit floating point number.
	B32, //!< A 32-bit boolean value.
};

/**
 * @brief The composition of a descriptor's field.
 *
 * ### Note on matrices
 *
 * The matrix layout used here matches the one used in GLSL, that is, a MatNxM matrix has N columns and M rows,
 * which is backward from convention in mathematics.
 */
enum HCComposition {
	Scalar, //!< A singular value.
	Vec2, //!< A 2 element vector/array of values.
	Vec3, //!< A 3 element vector/array of values.
	Vec4, //!< A 4 element vector/array of values.
	Mat2x2, //!< A 2x2 matrix of values.
	Mat2x3, //!< A 2x3 matrix of values.
	Mat2x4, //!< A 2x4 matrix of values.
	Mat3x2, //!< A 3x2 matrix of values.
	Mat3x3, //!< A 3x3 matrix of values.
	Mat3x4, //!< A 3x4 matrix of values.
	Mat4x2, //!< A 4x2 matrix of values.
	Mat4x3, //!< A 4x3 matrix of values.
	Mat4x4, //!< A 4x4 matrix of values.
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
	Unknown,
	Std140,
	Std430,
};

/**
 * @brief A data descriptor.
 *
 * This struct should never be created directly. Instead, `hc_create_descriptor` should be used to create a new
 * instance, and `hc_destroy_descriptor` used to destroy the instance.
 */
struct HCDescriptor {
	struct HCField *fields;
	size_t field_count; //!< The number of fields. This value MUST be treated as const and never be changed.
	enum HCAlignment alignment;
};

/**
 * @brief Create a new `HCDescriptor`.
 *
 * All fields in the new descriptor are uninitialised and should be assigned correct values after creation.
 *
 * @param field_count The number of fields in the descriptor.
 * @return The allocated descriptor.
 */
struct HCDescriptor hc_create_descriptor(size_t field_count);

/**
 * @brief Destroys a `HCDescriptor`.
 *
 * @param descriptor A pointer to the `HCDescriptor` to be destroyed.
 */
void hc_destroy_descriptor(struct HCDescriptor *descriptor);

#ifdef __cplusplus
}
#endif // __cplusplus
