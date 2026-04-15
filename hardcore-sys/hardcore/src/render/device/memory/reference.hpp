#pragma once

#include <vulkan/vulkan.h>

#include <util/number.hpp>

namespace hc::render::device::memory {
    struct Ref {
        u64 pool;
        VkDeviceSize pool_size;
        VkDeviceSize size;
        VkDeviceSize offset;
        VkDeviceSize padding;
        u32 flags;
    };

    struct BufferRef : Ref {
        VkBuffer buffer;
    };

    struct DynamicBufferRef : BufferRef {
        void** host_ptr;
    };
}
