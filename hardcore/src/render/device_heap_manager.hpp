#pragma once

#include "render_core.hpp"

namespace ENGINE_NAMESPACE
{
	class device_heap_manager
	{
	public:
		enum class heap : std::uint8_t
		{
			NONE,
			MAIN,
			DYNAMIC,
			UPLOAD,
			DOWNLOAD,
		};

		device_heap_manager() = default;
		device_heap_manager(VkPhysicalDevice physical_device);

		device_heap_manager(const device_heap_manager&) = delete;
		device_heap_manager& operator=(const device_heap_manager&) = delete;

		device_heap_manager(device_heap_manager&& other) noexcept :
			_mem_properties(std::exchange(other._mem_properties, {})),
			host_coherent(std::exchange(other.host_coherent, false)),
			main_type_idx(std::exchange(other.main_type_idx, std::numeric_limits<std::uint32_t>::max())),
			dynamic_type_idx(std::exchange(other.dynamic_type_idx, std::numeric_limits<std::uint32_t>::max())),
			upload_type_idx(std::exchange(other.upload_type_idx, std::numeric_limits<std::uint32_t>::max())),
			download_type_idx(std::exchange(other.download_type_idx, std::numeric_limits<std::uint32_t>::max()))
		{}

		inline device_heap_manager& operator=(device_heap_manager&& other) noexcept
		{
			_mem_properties = std::exchange(other._mem_properties, {});
			host_coherent = std::exchange(other.host_coherent, false);
			main_type_idx = std::exchange(other.main_type_idx, std::numeric_limits<std::uint32_t>::max());
			dynamic_type_idx = std::exchange(other.dynamic_type_idx, std::numeric_limits<std::uint32_t>::max());
			upload_type_idx = std::exchange(other.upload_type_idx, std::numeric_limits<std::uint32_t>::max());
			download_type_idx = std::exchange(other.download_type_idx, std::numeric_limits<std::uint32_t>::max());
			return *this;
		}

		void alloc_buffer(VkDevice device,
			VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size,
			VkBufferUsageFlags usage, heap h);

		std::uint32_t alloc_texture_memory(VkDevice device, VkDeviceMemory& memory, VkDeviceSize size,
			heap preferred_heap, std::uint32_t memory_type_bits);

		void free(VkDevice device, VkDeviceMemory& memory);

		const VkPhysicalDeviceMemoryProperties& mem_properties() const noexcept { return _mem_properties; }

		inline bool host_coherent_dynamic_heap() const noexcept 
		{
			return _mem_properties.memoryTypes[dynamic_type_idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		inline bool host_coherent_upload_heap() const noexcept
		{
			return _mem_properties.memoryTypes[upload_type_idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		std::uint32_t allocations() const noexcept { return _allocations; }

	private:
		std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);
		std::uint32_t get_memory_type_idx(heap h, std::uint32_t memory_type_bits) const;

		VkPhysicalDeviceMemoryProperties _mem_properties;

		std::uint32_t main_type_idx, dynamic_type_idx, upload_type_idx, download_type_idx;

		bool host_coherent = false;

		std::uint32_t _allocations = 0;
	};
}
