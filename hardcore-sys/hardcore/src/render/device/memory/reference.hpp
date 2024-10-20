#pragma once

#include <vulkan/vulkan.h>

namespace hc::render::device::memory {
	struct Ref {
		VkBuffer buffer;
		u64 pool;
		VkDeviceSize pool_size;
		VkDeviceSize size;
		VkDeviceSize offset;
		VkDeviceSize padding;
		VkBufferUsageFlags flags;
	};
}
