#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief Initialisation parameters for the rendering portion of the Hardcore context.
 */
struct HCRenderParams {
    uint32_t max_frames_in_flight; //!< The maximum number of frames that may be getting rendered at once.
};

#ifdef __cplusplus
}
#endif // __cplusplus
