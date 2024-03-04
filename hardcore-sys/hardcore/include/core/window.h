#pragma once

#ifndef HC_HEADLESS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief A high-level abstraction of a graphical window.
 */
struct HCWindow {
    void *handle; //!< The internal handle of this window.
};

/**
 * @brief Constructs a new `HCWindow`.
 *
 * @return The newly created `HCWindow`. The returned object is invalid if some error has occurred.
 */
struct HCWindow hc_new_window();

/**
 * @brief Destroys a `HCWindow`.
 *
 * @param window a pointer to the `HCWindow` to be destroyed
 */
void hc_destroy_window(struct HCWindow *window);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HC_HEADLESS

