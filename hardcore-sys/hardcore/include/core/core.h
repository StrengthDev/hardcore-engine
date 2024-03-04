#pragma once

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct HCInitParams {
    HCLogFn log_fn;
};

/**
 * @brief Initialise the library.
 *
 * This function must be called before any other library functions may be used.
 *
 * @param params The initialisation parameters
 *
 * @return 0 on success, a negative code on if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_init(struct HCInitParams params);

/**
 * @brief Terminate the library.
 *
 * Once this function is called, `hc_init()` must be called once again before other library functions.
 *
 * @return 0 on success, a negative code on if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_term();

#ifdef __cplusplus
}
#endif // __cplusplus
