#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief A color value.
 *
 * All components should be values within the range [0, 1].
 */
struct HCColor {
    float r; //!< The red color component.
    float g; //!< The green color component.
    float b; //!< The blue color component.
    float a; //!< The alpha color component.
};

#ifdef __cplusplus
}
#endif // __cplusplus