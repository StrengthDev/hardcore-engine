#pragma once

#include "descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief The type of a data buffer.
 */
enum HCBufferKind {
	/**
	* @brief A buffer which contains vertices.
	*
	* Used in raster draw commands.
	*/
	Vertex,
	/**
	* @brief A buffer which contains indexes of a vertex buffer.
	*
	* Used in raster draw commands.
	*/
	Index,
	/**
	* @brief A buffer containing arbitrary data.
	*
	* In shaders, this type of buffer is read-only.
	* The amount of memory each uniform buffer can take is very limited.
	*/
	Uniform,
	/**
	* @brief A buffer containing arbitrary data.
	*
	* Slightly slower than uniform buffers.
	* Unlike uniform buffers, this type of buffer can be much larger and written to in shaders.
	*/
	Storage,
};

/**
 * @brief A device buffer.
 *
 * Every field of this struct MUST be treated as const, and thus, not altered throughout its lifetime.
 *
 * This struct should never be created directly. Instead, `hc_new_buffer` or `hc_new_index_buffer` should be used to
 * create a new instance, and `hc_destroy_buffer` used to destroy the instance.
 */
struct HCBuffer {
	uint64_t id; //!< The ID of this buffer within the device.
	size_t size; //!< The amount of usable memory occupied by this buffer, in bytes.
	uint32_t device; //!< The ID of the device which this buffer belongs to.
};

struct HCBuffer
hc_new_buffer(enum HCBufferKind kind, const struct HCDescriptor *descriptor, uint64_t count, bool writable,
			uint32_t device);

struct HCBuffer hc_new_index_buffer(enum HCPrimitive index_type, uint64_t count, bool writable, uint32_t device);

void hc_destroy_buffer(struct HCBuffer *buffer);

/**
 * @brief A dynamic device buffer.
 *
 * Unlike regular buffers, dynamic buffers may be written to directly by the host.
 *
 * Just like is the case with `HCRDataBuffer`, every field of this struct MUST be treated as const, and thus, not
 * altered throughout its lifetime.
 *
 * This struct should never be created directly. Instead, `hc_new_dynamic_buffer` or `hc_new_dynamic_index_buffer`
 * should be used to create a new instance, and `hc_destroy_dynamic_buffer` used to destroy the instance.
 */
struct HCDynamicBuffer {
	uint64_t id; //!< The ID of this buffer within the device.
	size_t size; //!< The amount of usable memory occupied by this buffer, in bytes.
	void **data; //!< A pointer to the underlying buffer backing this buffer's data.
	size_t data_offset; //!< The offset to this buffer's data within the underlying buffer which backs this one.
	uint32_t device; //!< The ID of the device which this buffer belongs to.
};

struct HCDynamicBuffer
hc_new_dynamic_buffer(enum HCBufferKind kind, const struct HCDescriptor *descriptor, uint64_t count,
					bool writable, uint32_t device);

struct HCDynamicBuffer hc_new_dynamic_index_buffer(enum HCPrimitive index_type, uint64_t count, bool writable,
													uint32_t device);

void hc_destroy_dynamic_buffer(struct HCDynamicBuffer *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
