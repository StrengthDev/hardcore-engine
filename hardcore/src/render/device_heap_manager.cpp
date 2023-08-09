#include <pch.hpp>

#include <render/device_heap_manager.hpp>

const VkMemoryPropertyFlags main_required_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
const VkMemoryPropertyFlags main_unwanted_flags = 
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

const VkMemoryPropertyFlags dynamic_required_flags = 
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
const VkMemoryPropertyFlags dynamic_unwanted_flags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

const VkMemoryPropertyFlags upload_required_flags = 
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags upload_unwanted_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

const VkMemoryPropertyFlags download_required_flags = 
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
const VkMemoryPropertyFlags download_unwanted_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

namespace ENGINE_NAMESPACE
{
	device_heap_manager::device_heap_manager(VkPhysicalDevice physical_device)
	{
		vkGetPhysicalDeviceMemoryProperties(physical_device, &m_mem_properties);

		constexpr u32 unassigned_idx = std::numeric_limits<u32>::max();
		m_main_type_idx = unassigned_idx;
		m_dynamic_type_idx = unassigned_idx;
		m_upload_type_idx = unassigned_idx;
		m_download_type_idx = unassigned_idx;

		for (u32 i = 0; i < m_mem_properties.memoryTypeCount; i++)
		{
			VkMemoryPropertyFlags flags = m_mem_properties.memoryTypes[i].propertyFlags;

			LOG_INTERNAL_INFO("[RENDERER] Memory type " << i << " properties: "
				<< '(' << std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(flags) << ") => "
				<< (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED " : "")
				<< (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED " : "")
				<< (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED " : ""));
		
			if (m_main_type_idx == unassigned_idx || 
				(m_main_type_idx != unassigned_idx && 
					m_mem_properties.memoryTypes[m_main_type_idx].propertyFlags != main_required_flags))
			{
				if (flags == main_required_flags)
				{
					m_main_type_idx = i;
					continue;
				}

				if (m_main_type_idx == unassigned_idx && flags & main_required_flags && !(flags & main_unwanted_flags))
					m_main_type_idx = i;
			}

			if (m_dynamic_type_idx == unassigned_idx ||
				(m_dynamic_type_idx != unassigned_idx &&
					m_mem_properties.memoryTypes[m_dynamic_type_idx].propertyFlags != 
					(dynamic_required_flags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
			{
				if (flags == (dynamic_required_flags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				{
					m_dynamic_type_idx = i;
					continue;
				}

				if (m_dynamic_type_idx == unassigned_idx && flags & dynamic_required_flags && !(flags & dynamic_unwanted_flags))
					m_dynamic_type_idx = i;
			}

			if (m_upload_type_idx == unassigned_idx ||
				(m_upload_type_idx != unassigned_idx &&
					m_mem_properties.memoryTypes[m_upload_type_idx].propertyFlags != upload_required_flags))
			{
				if (flags == upload_required_flags)
				{
					m_upload_type_idx = i;
					continue;
				}

				if (m_upload_type_idx == unassigned_idx && flags & upload_required_flags && !(flags & upload_unwanted_flags))
					m_upload_type_idx = i;
			}

			if (m_download_type_idx == unassigned_idx ||
				(m_download_type_idx != unassigned_idx &&
					m_mem_properties.memoryTypes[m_download_type_idx].propertyFlags != download_required_flags))
			{
				if (flags == download_required_flags)
				{
					m_download_type_idx = i;
					continue;
				}

				if (m_download_type_idx == unassigned_idx && flags & download_required_flags && !(flags & download_unwanted_flags))
					m_download_type_idx = i;
			}
		}

		LOG_INTERNAL_INFO("[RENDERER] Heap type indexes: main = " << m_main_type_idx << " ; dynamic = " 
			<< m_dynamic_type_idx << " ; upload = " << m_upload_type_idx << " ; download = " << m_download_type_idx);

		if (host_coherent_dynamic_heap())
		{
			LOG_INTERNAL_INFO("[RENDERER] Dynamic heap is host coherent");
		}
		else
		{
			LOG_INTERNAL_INFO("[RENDERER] Dynamic heap is NOT host coherent");
		}

		if (host_coherent_upload_heap())
		{
			LOG_INTERNAL_INFO("[RENDERER] Upload heap is host coherent");
		}
		else
		{
			LOG_INTERNAL_INFO("[RENDERER] Upload heap is NOT host coherent");
		}
	}

	u32 device_heap_manager::find_memory_type(u32 type_filter, VkMemoryPropertyFlags heap_properties)
	{
		//Exact type search
		for (u32 i = 0; i < m_mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && m_mem_properties.memoryTypes[i].propertyFlags == heap_properties)
			{
				return i;
			}
		}
		LOG_INTERNAL_WARN("Failed to find exact memory type.");

		//Relaxed search
		for (u32 i = 0; i < m_mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && (m_mem_properties.memoryTypes[i].propertyFlags & heap_properties) == heap_properties)
			{
				return i;
			}
		}

		//not sure how to deal with this
		LOG_INTERNAL_ERROR("Failed to find suitable memory type. Type bit mask: "
			<< std::bitset<sizeof(type_filter) * 8>(type_filter) << " Property flags: "
			<< std::bitset<sizeof(heap_properties) * 8>(heap_properties));
		CRASH("Could not find suitable memory type");
		return std::numeric_limits<u32>::max();
	}

	u32 device_heap_manager::get_memory_type_idx(heap heap, u32 memory_type_bits) const
	{
		u32 res = 0;

		switch (heap)
		{
		case heap::MAIN:		res = m_main_type_idx;		break;
		case heap::DYNAMIC:		res = m_dynamic_type_idx;	break;
		case heap::UPLOAD:		res = m_upload_type_idx;	break;
		case heap::DOWNLOAD:	res = m_download_type_idx;	break;
		default:
			CRASH("Invalid heap");
			break;
		}
		//TODO this really shouldnt be just an assert, im assuming the chosen heap always meets the requirements
		INTERNAL_ASSERT(memory_type_bits & 1U << res, "Memory requirements not met");

		return res;
	}

	void device_heap_manager::alloc_buffer(VkDevice device,
		VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size,
		VkBufferUsageFlags usage, heap heap)
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CRASH_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "Failed to create buffer");

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = memory_requirements.size;
		memory_info.memoryTypeIndex = get_memory_type_idx(heap, memory_requirements.memoryTypeBits);

		VK_CRASH_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		vkBindBufferMemory(device, buffer, memory, 0);

		m_allocations++;
	}

	u32 device_heap_manager::alloc_texture_memory(VkDevice device, VkDeviceMemory& memory, VkDeviceSize size,
		heap preferred_heap, u32 memory_type_bits)
	{
		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = size;
		memory_info.memoryTypeIndex = get_memory_type_idx(preferred_heap, memory_type_bits);

		VK_CRASH_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		m_allocations++;

		return memory_info.memoryTypeIndex;
	}

	void device_heap_manager::free(VkDevice device, VkDeviceMemory& memory)
	{
		//having to call free from the heap manager instead of freeing the memory directly is a bit silly, but since
		//the number of allocations will be counted for debugging and profiling, may aswell do it like this
		//it also falls more inline with the purpose of the heap manager
		vkFreeMemory(device, memory, nullptr);
		memory = VK_NULL_HANDLE;
		m_allocations--;
	}
}
