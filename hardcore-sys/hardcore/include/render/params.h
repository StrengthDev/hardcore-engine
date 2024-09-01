#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

#include "../core/log.h"

// Vulkan debug callback scope bits.
// Match original values.

/**
 * Designates general or miscellaneous events.
 */
const int HC_VK_GENERAL = 0x00000001;

/**
 * Designates an event emitted from validation layers.
 */
const int HC_VK_VALIDATION = 0x00000002;

/**
 * Designates an event concerning performance.
 */
const int HC_VK_PERFORMANCE = 0x00000004;

/**
 * Designates events concerning device address bindings.
 */
const int HC_VK_DEVICE_ADDRESS_BINDING = 0x00000008;

/**
 * @brief The type/signature of a Vulkan debug callback function.
 *
 * This kind of function is used internally by Vulkan to debug various events.
 *
 * @param kind - The severity of the callback's event.
 * @param scope - The scope of the callback's event.
 * @param text - The callback message.
 */
typedef void (*HCVulkanDebugCallbackFn)(enum HCLogKind kind, int scope, const char *text);

/**
 * @brief Initialisation parameters for the rendering portion of the Hardcore context.
 */
struct HCRenderParams {
    uint32_t max_frames_in_flight; //!< The maximum number of frames that may be getting rendered at once.
    HCVulkanDebugCallbackFn debug_callback; //!< The function that Vulkan should use for emitting debug events.
};

#ifdef __cplusplus
}
#endif // __cplusplus
