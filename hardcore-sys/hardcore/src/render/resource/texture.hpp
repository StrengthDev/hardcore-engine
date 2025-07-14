#pragma once

#include <core/error.hpp>
#include <util/number.hpp>

namespace hc::render::texture {
    /**
    * @brief The inner parameters of a texture.
    */
    struct Params {
        u64 id; //!< The ID of this texture within the device.
        Sz size; //!< The amount of usable memory occupied by this texture, in bytes.
    };

    std::expected<VkImage, Error> create_image(
        VkPhysicalDevice physical_device,
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkImageCreateInfo const& image_info
    );

    std::expected<VkImage, Error> create_image_view(
        VolkDeviceTable const& fn_table,
        VkDevice device,
        VkImage image,
        VkImageCreateInfo image_info
    );
}
