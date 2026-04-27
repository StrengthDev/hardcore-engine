#pragma once

#include "log.h"
#include "result.h"
#include "version.h"

#include "../render/params.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief A descriptor used to identify an application by its name and version.
 */
struct HCApplicationDescriptor {
    const char* name; //!< The name of the application.
    struct HCVersion version; //!< The application's version.
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
 * @brief Initialize the library.
 *
 * This function must be called before any other library functions may be used.
 *
 * @param params The initialization parameters.
 *
 * @return Result value object.
 */
struct HCResult hc_init(struct HCInitParams params);

/**
 * @brief Terminate the library.
 *
 * Once this function is called, `hc_init()` must be called once again before other library functions.
 */
void hc_term();

#ifdef __cplusplus
}
#endif // __cplusplus
