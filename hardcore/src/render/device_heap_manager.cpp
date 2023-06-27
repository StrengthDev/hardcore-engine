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
		vkGetPhysicalDeviceMemoryProperties(physical_device, &_mem_properties);

		constexpr std::uint32_t unassigned_idx = std::numeric_limits<std::uint32_t>::max();
		main_type_idx = unassigned_idx;
		dynamic_type_idx = unassigned_idx;
		upload_type_idx = unassigned_idx;
		download_type_idx = unassigned_idx;

		for (std::uint32_t i = 0; i < _mem_properties.memoryTypeCount; i++)
		{
			VkMemoryPropertyFlags flags = _mem_properties.memoryTypes[i].propertyFlags;

			LOG_INTERNAL_INFO("[RENDERER] Memory type " << i << " properties: "
				<< '(' << std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(flags) << ") => "
				<< (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT " : "")
				<< (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED " : "")
				<< (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED " : "")
				<< (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED " : ""));
		
			if (main_type_idx == unassigned_idx || 
				(main_type_idx != unassigned_idx && 
					_mem_properties.memoryTypes[main_type_idx].propertyFlags != main_required_flags))
			{
				if (flags == main_required_flags)
				{
					main_type_idx = i;
					continue;
				}

				if (main_type_idx == unassigned_idx && flags & main_required_flags && !(flags & main_unwanted_flags))
					main_type_idx = i;
			}

			if (dynamic_type_idx == unassigned_idx ||
				(dynamic_type_idx != unassigned_idx &&
					_mem_properties.memoryTypes[dynamic_type_idx].propertyFlags != 
					(dynamic_required_flags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
			{
				if (flags == (dynamic_required_flags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				{
					dynamic_type_idx = i;
					continue;
				}

				if (dynamic_type_idx == unassigned_idx && flags & dynamic_required_flags && !(flags & dynamic_unwanted_flags))
					dynamic_type_idx = i;
			}

			if (upload_type_idx == unassigned_idx ||
				(upload_type_idx != unassigned_idx &&
					_mem_properties.memoryTypes[upload_type_idx].propertyFlags != upload_required_flags))
			{
				if (flags == upload_required_flags)
				{
					upload_type_idx = i;
					continue;
				}

				if (upload_type_idx == unassigned_idx && flags & upload_required_flags && !(flags & upload_unwanted_flags))
					upload_type_idx = i;
			}

			if (download_type_idx == unassigned_idx ||
				(download_type_idx != unassigned_idx &&
					_mem_properties.memoryTypes[download_type_idx].propertyFlags != download_required_flags))
			{
				if (flags == download_required_flags)
				{
					download_type_idx = i;
					continue;
				}

				if (download_type_idx == unassigned_idx && flags & download_required_flags && !(flags & download_unwanted_flags))
					download_type_idx = i;
			}
		}

		LOG_INTERNAL_INFO("[RENDERER] Heap type indexes: main = " << main_type_idx << " ; dynamic = " 
			<< dynamic_type_idx << " ; upload = " << upload_type_idx << " ; download = " << download_type_idx);

		if (host_coherent_dynamic_heap())
		{
			LOG_INTERNAL_INFO("[RENDERER] Dynamic heap is host coherent");
		}
		else
		{
			LOG_INTERNAL_INFO("[RENDERER] Dynamic heap is NOT host coherent");
		}
	}

	std::uint32_t device_heap_manager::find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags heap_properties)
	{
		//Exact type search
		for (std::uint32_t i = 0; i < _mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && _mem_properties.memoryTypes[i].propertyFlags == heap_properties)
			{
				return i;
			}
		}
		LOG_INTERNAL_WARN("Failed to find exact memory type.");

		//Relaxed search
		for (std::uint32_t i = 0; i < _mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && (_mem_properties.memoryTypes[i].propertyFlags & heap_properties) == heap_properties)
			{
				return i;
			}
		}

		//not sure how to deal with this
		LOG_INTERNAL_ERROR("Failed to find suitable memory type. Type bit mask: "
			<< std::bitset<sizeof(type_filter) * 8>(type_filter) << " Property flags: "
			<< std::bitset<sizeof(heap_properties) * 8>(heap_properties));
		CRASH("Could not find suitable memory type");
		return std::numeric_limits<std::uint32_t>::max();
	}

	void device_heap_manager::alloc_buffer(VkDevice device,
		VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size,
		VkBufferUsageFlags usage, heap h)
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

		switch (h)
		{
		case hc::device_heap_manager::heap::MAIN:
			memory_info.memoryTypeIndex = main_type_idx;
			break;
		case hc::device_heap_manager::heap::DYNAMIC:
			memory_info.memoryTypeIndex = dynamic_type_idx;
			break;
		case hc::device_heap_manager::heap::UPLOAD:
			memory_info.memoryTypeIndex = upload_type_idx;
			break;
		case hc::device_heap_manager::heap::DOWNLOAD:
			memory_info.memoryTypeIndex = download_type_idx;
			break;
		default:
			CRASH("Invalid heap");
			break;
		}
		//TODO this really shouldnt be just an assert, im assuming the chosen heap always meets the requirements
		INTERNAL_ASSERT(memory_requirements.memoryTypeBits & 1U << memory_info.memoryTypeIndex,
			"Memory requirements not met");

		VK_CRASH_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		vkBindBufferMemory(device, buffer, memory, 0);

		_allocations++;
	}

	std::uint32_t device_heap_manager::alloc_texture_memory(VkDevice device, VkDeviceMemory& memory, VkDeviceSize size,
		std::uint32_t memory_type_bits, VkMemoryPropertyFlags heap_properties)
	{
		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = size;
		memory_info.memoryTypeIndex = find_memory_type(memory_type_bits, heap_properties);

		VK_CRASH_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		_allocations++;

		return memory_info.memoryTypeIndex;
	}

	void device_heap_manager::free(VkDevice device, VkDeviceMemory& memory)
	{
		//having to call free from the heap manager instead of freeing the memory directly is a bit silly, but since
		//the number of allocations will be counted for debugging and profiling, may aswell do it like this
		//it also falls more inline with the purpose of the heap manager
		vkFreeMemory(device, memory, nullptr);
		_allocations--;
	}

	void alloc_image()
	{
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.pNext = nullptr;
		image_info.flags;
		image_info.imageType;
		image_info.format;
		image_info.extent;
		image_info.mipLevels;
		image_info.arrayLayers;
		image_info.samples;
		image_info.tiling;
		image_info.usage;
		image_info.sharingMode;
		image_info.queueFamilyIndexCount;
		image_info.pQueueFamilyIndices;
		image_info.initialLayout;

		//vkCreateImage(radd.device, &image_info, nullptr, &image)

		VkMemoryRequirements memory_requirements;
		//vkGetImageMemoryRequirements(radd.device, image, &memory_requirements);

		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = memory_requirements.size;
		//memory_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, heap_properties);

		//VK_CRASH_CHECK(vkAllocateMemory(device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		//vkBindImageMemory(radd.device, image, memory, 0);
	}
}
