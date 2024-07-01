#pragma once

// Can't use angled include here due to bindgen
#include "../render/params.h"

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief A descriptor used to identify an application by its name and version.
 */
struct HCApplicationDescriptor {
    const char *name; //!< The name of the application.
    uint32_t major; //!< The major version of the application.
    uint32_t minor; //!< The minor version of the application.
    uint32_t patch; //!< The patch version of the application.
};

/**
 * @brief Initialisation parameters for a new Hardcore context.
 */
struct HCInitParams {
    struct HCApplicationDescriptor app; //!< A descriptor of the currently running program/application.
    struct HCRenderParams render_params; //!< Initialisation parameters for the rendering portion of the Hardcore context.
    HCLogFn log_fn; //!< The function the context should use for emitting log events.
    HCStartSpanFn start_span_fn; //!< The function the context should use for starting spans.
    HCEndSpanFn end_span_fn; //!< The function the context should use for ending spans.
};

/**
 * @brief Initialise the library.
 *
 * This function must be called before any other library functions may be used.
 *
 * @param params The initialisation parameters.
 *
 * @return 0 on success, a negative code if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_init(struct HCInitParams params);

/**
 * @brief Terminate the library.
 *
 * Once this function is called, `hc_init()` must be called once again before other library functions.
 *
 * @return 0 on success, a negative code if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_term();

#ifdef __cplusplus
}
#endif // __cplusplus
