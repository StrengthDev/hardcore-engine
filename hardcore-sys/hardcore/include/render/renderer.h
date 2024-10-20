#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
* @brief Process one frame.
*
* This function is meant to be called in a loop.
* In non-headless configurations, it can (and should) be called from a loop seperated from `hc_poll_events()`,
* in another thread.
*
* @return 0 on success, a negative code if an error occurs and a code larger than 0 on success, but with some
* warning.
*/
int hc_render_tick();

/**
 * @brief Clean up the renderer context.
 *
 * This function should be called after exiting out of the rendering loop (whichever loop calls `hc_render_tick`). It
 * will destroy and clean any remaining rendering resources that didn't get properly freed after being marked for
 * destruction.
 *
 * @return 0 on success, a negative code if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_render_finish();

#ifdef __cplusplus
}
#endif // __cplusplus
