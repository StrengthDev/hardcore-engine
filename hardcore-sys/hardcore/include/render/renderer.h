#pragma once

#include "../core/result.h"
#include "../core/version.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern struct HCVersion const HC_VULKAN_API_VERSION; //!< The version of the Vulkan API that is used.
extern struct HCVersion const HC_VULKAN_HEADERS_VERSION; //!< The version of the Vulkan headers that was compiled.
extern uint32_t const HC_VOLK_HEADER_VERSION; //!< The version of the Volk header that was compiled.

/**
* @brief Process one frame.
*
* This function is meant to be called in a loop.
* In non-headless configurations, it can (and should) be called from a loop seperated from `hc_poll_events()`,
* in another thread.
*
* @return Result value object.
*/
struct HCResult hc_render_tick();

/**
 * @brief Clean up the renderer context.
 *
 * This function should be called after exiting out of the rendering loop (whichever loop calls `hc_render_tick`). It
 * will destroy and clean any remaining rendering resources that didn't get properly freed after being marked for
 * destruction.
 *
 * @return Result value object.
 */
struct HCResult hc_render_finish();

#ifdef __cplusplus
}
#endif // __cplusplus
