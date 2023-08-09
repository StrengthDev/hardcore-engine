#pragma once

#include "render_core.hpp"

namespace ENGINE_NAMESPACE
{
	class device_heap_manager
	{
	public:
		enum class heap : u8
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
			m_mem_properties(std::exchange(other.m_mem_properties, {})),
			m_main_type_idx(std::exchange(other.m_main_type_idx, std::numeric_limits<u32>::max())),
			m_dynamic_type_idx(std::exchange(other.m_dynamic_type_idx, std::numeric_limits<u32>::max())),
			m_upload_type_idx(std::exchange(other.m_upload_type_idx, std::numeric_limits<u32>::max())),
			m_download_type_idx(std::exchange(other.m_download_type_idx, std::numeric_limits<u32>::max()))
		{}

		inline device_heap_manager& operator=(device_heap_manager&& other) noexcept
		{
			m_mem_properties = std::exchange(other.m_mem_properties, {});
			m_main_type_idx = std::exchange(other.m_main_type_idx, std::numeric_limits<u32>::max());
			m_dynamic_type_idx = std::exchange(other.m_dynamic_type_idx, std::numeric_limits<u32>::max());
			m_upload_type_idx = std::exchange(other.m_upload_type_idx, std::numeric_limits<u32>::max());
			m_download_type_idx = std::exchange(other.m_download_type_idx, std::numeric_limits<u32>::max());
			return *this;
		}

		void alloc_buffer(VkDevice device,
			VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size,
			VkBufferUsageFlags usage, heap heap);

		u32 alloc_texture_memory(VkDevice device, VkDeviceMemory& memory, VkDeviceSize size,
			heap preferred_heap, u32 memory_type_bits);

		void free(VkDevice device, VkDeviceMemory& memory);

		const VkPhysicalDeviceMemoryProperties& mem_properties() const noexcept { return m_mem_properties; }

		inline bool host_coherent_dynamic_heap() const noexcept 
		{
			return m_mem_properties.memoryTypes[m_dynamic_type_idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		inline bool host_coherent_upload_heap() const noexcept
		{
			return m_mem_properties.memoryTypes[m_upload_type_idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		u32 allocations() const noexcept { return m_allocations; }

	private:
		u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);
		u32 get_memory_type_idx(heap heap, u32 memory_type_bits) const;

		VkPhysicalDeviceMemoryProperties m_mem_properties;

		u32 m_main_type_idx, m_dynamic_type_idx, m_upload_type_idx, m_download_type_idx;

		u32 m_allocations = 0;
	};
}
