#pragma once

#include <vulkan/vulkan.h>

namespace hc::render::device::memory {
    struct Ref {
        VkDeviceSize size;
        VkDeviceSize offset;
        VkBufferUsageFlags flags;
    };
}
