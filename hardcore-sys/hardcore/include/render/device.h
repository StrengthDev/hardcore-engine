#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief Get the number of devices found.
 *
 * @return The number of devices, or 0 if no context is currently initialised.
 */
uint32_t hc_device_count();

/**
 * @brief Get the name of a device.
 *
 * @param device The index of the device. This should be a number between 0 (inclusive) and the device count (exclusive).
 * @return The name of the device, or a *nullptr* if any error occurs.
 */
const char *hc_device_name(uint32_t device);

#ifdef __cplusplus
}
#endif // __cplusplus
