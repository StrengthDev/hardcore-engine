#pragma once

#include <vulkan/vulkan.h>

namespace hc::render::device::memory {
    struct Ref {
        u64 pool;
        VkDeviceSize size;
        VkDeviceSize offset;
        VkDeviceSize padding;
        VkBufferUsageFlags flags;
    };
}
