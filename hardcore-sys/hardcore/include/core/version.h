#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>

/**
 * @brief A version value.
 */
struct HCVersion {
    uint32_t major; //!< The major version.
    uint32_t minor; //!< The minor version.
    uint32_t patch; //!< The patch version.
};

#ifdef __cplusplus
}
#endif // __cplusplus
