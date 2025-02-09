#pragma once

#include <vulkan/vulkan.h>

namespace hc::render {
    /**
    * @brief Returns a C string representation of the provided `VkResult`.
    *
    * @param result The `VKResult` for which the string is returned.
    * @return A C string representation of the `VKResult`.
    */
    const char* to_str(VkResult result);

    /**
    * @brief Returns a string representation of the provided `VkExtent2D`.
    *
    * @param extent The `VkExtent2D` for which the string is returned.
    * @return A string representation of the `VkExtent2D`.
    */
    std::string to_str(const VkExtent2D& extent);
}
