#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialise the library.
 *
 * This function must be called before any other library functions may be used.
 *
 * @return 0 on success, a negative code on if an error occurs and a code larger than 0 on success, but with some
 * warning.
 */
int hc_init();

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
