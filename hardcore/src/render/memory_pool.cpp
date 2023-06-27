#include <pch.hpp>

#include <render/memory_pool.hpp>

namespace ENGINE_NAMESPACE
{
	const std::uint32_t initial_pool_slot_size = 16;

	memory_pool::memory_pool(VkDeviceSize size, bool per_frame_allocation) : _size(size)
	{
		n_slots = initial_pool_slot_size;
		slots = t_malloc<memory_slot>(initial_pool_slot_size);
		slots[0].in_use = false;
		slots[0].offset = 0;
		slots[0].size = size;
		largest_free_slot = 0;

		for (std::uint32_t i = 1; i < n_slots; i++)
		{
			slots[i].in_use = false;
			slots[i].offset = size;
			slots[i].size = 0;
		}

		this->per_frame_allocation = per_frame_allocation;
	}

	memory_pool::~memory_pool()
	{
		INTERNAL_ASSERT(!slots, "Memory pool not freed");
	}

	void memory_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		if (slots)
		{
			heap_manager.free(device, _memory);
			std::free(slots);
			slots = nullptr;
		}
	}

	bool memory_pool::search(VkDeviceSize size, VkDeviceSize alignment, 
		std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed) const
	{
		if (_size < size) return false;

		//largest free slot is unknown, must check everything, look for smallest possible fit
		//if possible, update largest free slot to save some performance
		if (slots[largest_free_slot].in_use)
		{
			VkDeviceSize largest_fit = 0;
			std::uint32_t largest_idx = 0;
			VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
			std::uint32_t smallest_idx = std::numeric_limits<std::uint32_t>::max();
			for (std::uint32_t slot_idx = 0; slot_idx < n_slots; slot_idx++)
			{
				if (!slots[slot_idx].in_use)
				{
					if (largest_fit < slots[slot_idx].size)
					{
						largest_fit = slots[slot_idx].size;
						largest_idx = slot_idx;
					}
					if ((size + alignment_pad(slots[slot_idx].offset, alignment)) <= slots[slot_idx].size 
						&& smallest_fit > slots[slot_idx].size)
					{
						smallest_fit = slots[slot_idx].size;
						smallest_idx = slot_idx;
					}
				}
			}
			VkDeviceSize size_needed = (size + alignment_pad(slots[smallest_idx].offset, alignment));
			if (size_needed <= smallest_fit)
			{
				*out_slot_idx = smallest_idx;
				*out_size_needed = size_needed;
				if (!(largest_idx == smallest_idx))
					largest_free_slot = largest_idx;
				return true;
			}
			else
			{
				largest_free_slot = largest_idx;
				return false;
			}
		}

		//largest free slot is known, so if it is large enough, assign a slot in this pool, otherwise move on to next pool
		else
		{
			if (slots[largest_free_slot].size < size)
				return false;

			VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
			std::uint32_t smallest_idx = std::numeric_limits<std::uint32_t>::max();
			for (std::uint32_t slot_idx = 0; slot_idx < n_slots; slot_idx++)
			{
				if (!slots[slot_idx].in_use 
					&& (size + alignment_pad(slots[slot_idx].offset, alignment)) <= slots[slot_idx].size 
					&& smallest_fit > slots[slot_idx].size)
				{
					smallest_fit = slots[slot_idx].size;
					smallest_idx = slot_idx;
				}
			}

			if (smallest_fit == std::numeric_limits<VkDeviceSize>::max()) return false;

			*out_slot_idx = smallest_idx;
			*out_size_needed = size + alignment_pad(slots[smallest_idx].offset, alignment);
			return true;
		}
		return false;
	}

	void memory_pool::fill_slot(std::uint32_t slot_idx, VkDeviceSize size)
	{
		INTERNAL_ASSERT(slot_idx < n_slots, "Slot index out of bounds");
		INTERNAL_ASSERT(size <= slots[slot_idx].size, "Allocation size greater than slot size");
		INTERNAL_ASSERT(slot_idx == 0 ? true : slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use.");

		if (size < slots[slot_idx].size)
		{
			const std::uint32_t old_size = n_slots;
			bool selected_last = slot_idx == (n_slots - 1);

			//resize needed
			if (selected_last || (slots[slot_idx + 1].in_use && slots[n_slots - 1].in_use))
			{
				n_slots += initial_pool_slot_size; //TODO check if this is the best way to increment
				realloc_slots(n_slots);
				for (std::uint32_t i = old_size; i < n_slots; i++)
				{
					slots[i].in_use = false;
					slots[i].offset = slots[old_size - 1].offset + slots[old_size - 1].size;
					slots[i].size = 0;
				}
			}

			INTERNAL_ASSERT(slots[slot_idx + 1].in_use || !slots[slot_idx + 1].size,
				"Slot after the selected slot must be either empty or in use");

			//shift remaining slots one position and add new slot with the remaining unused memory
			if (slots[slot_idx + 1].in_use)
			{
				std::uint32_t last_in_use = 0;
				for (std::uint32_t i = old_size - 1; i > slot_idx; i--)
				{
					if (slots[i].in_use)
					{
						last_in_use = i;
						break;
					}
				}
				move_slots(slot_idx + 2, slot_idx + 1, last_in_use - slot_idx);
			}

			slots[slot_idx + 1].in_use = false;
			slots[slot_idx + 1].offset = slots[slot_idx].offset + size;
			slots[slot_idx + 1].size = slots[slot_idx].size - size;
		}

		slots[slot_idx].in_use = true;
		slots[slot_idx].size = size;
	}

	buffer_pool::buffer_pool(VkDevice device, device_heap_manager& heap_manager,
		VkDeviceSize size, VkBufferUsageFlags usage, device_heap_manager::heap h, 
		bool per_frame_allocation) :
		memory_pool(size, per_frame_allocation)
	{
		heap_manager.alloc_buffer(device, _memory, _buffer, 
			per_frame_allocation ? size * max_frames_in_flight : size, 
			usage, h);
	}

	void buffer_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		if (slots)
		{
			vkDestroyBuffer(device, _buffer, nullptr);
			memory_pool::free(device, heap_manager);
		}
	}

	void buffer_pool::realloc_slots(std::uint32_t new_count)
	{
		slots = t_realloc<memory_slot>(slots, new_count);
	}

	void buffer_pool::move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count)
	{
		std::memmove(&slots[dst], &slots[src], sizeof(memory_slot) * count);
	}

	void dynamic_buffer_pool::map(VkDevice device, std::uint8_t current_frame)
	{
		VK_CRASH_CHECK(vkMapMemory(device, _memory, _size * current_frame, _size, 0, &_host_ptr), "Failed to map memory");
	}

	void dynamic_buffer_pool::unmap(VkDevice device)
	{
		INTERNAL_ASSERT(_host_ptr != nullptr, "Memmory not mapped");
		vkUnmapMemory(device, _memory);
		_host_ptr = nullptr;
	}

	texture create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements* out_memory_requirements)
	{
		VkImage image;
		VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &image), "Failed to create image");

		vkGetImageMemoryRequirements(device, image, out_memory_requirements);

		VkImageView image_view;

		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.flags = 0;
		view_info.image = image;
		view_info.format = image_info.format;

		//setting all components to identity so no swizzling occurs
		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = image_info.mipLevels;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = image_info.arrayLayers;

		const bool is_cube = image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		INTERNAL_ASSERT(!is_cube || (is_cube && image_info.imageType == VK_IMAGE_TYPE_2D), 
			"Image type must be 2D if image is a cube");
		INTERNAL_ASSERT(!is_cube || (is_cube && image_info.arrayLayers % 6 == 0),
			"Number of image layers must be a multiple of 6 if image is a cube");

		const bool is_array = is_cube ? 1 < image_info.arrayLayers / 6 : 1 < image_info.arrayLayers;
		INTERNAL_ASSERT((is_array && image_info.imageType == VK_IMAGE_TYPE_3D), "3D image arrays are invalid");

		switch (image_info.imageType)
		{
		case VK_IMAGE_TYPE_1D:
			view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
			break;
		case VK_IMAGE_TYPE_2D:
			if (is_cube) view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
			else view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			break;
		case VK_IMAGE_TYPE_3D:
			view_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
			break;
		default:
			INTERNAL_ASSERT(false, "Invalid image type");
			break;
		}

		VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &image_view), "Failed to create image view");

		return { image, image_view };
	}

	texture_pool::texture_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size,
		std::uint32_t memory_type_bits, VkMemoryPropertyFlags heap_properties) :
		memory_pool(size)
	{
		memory_type_idx = heap_manager.alloc_texture_memory(device, _memory, size, memory_type_bits, heap_properties);
	}

	bool texture_pool::search(VkDeviceSize size, VkDeviceSize alignment, std::uint32_t memory_type_bits,
		std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed)
	{
		if (memory_type_bits & (1U << memory_type_idx))
			return memory_pool::search(size, alignment, out_slot_idx, out_size_needed);

		return false;
	}

	void texture_pool::fill_slot(VkDevice device, texture&& tex, std::uint32_t slot_idx, 
		VkDeviceSize size, VkDeviceSize alignment)
	{
		memory_pool::fill_slot(slot_idx, size);

		//memory_pool::fill_slot should perform any reallocation if necessary, so the slot should always exist

		VK_CRASH_CHECK(vkBindImageMemory(device, texture_slots[slot_idx].image, _memory,
			aligned_offset(slots[slot_idx].offset, alignment)), "Failed to bind image to memory");
		texture_slots[slot_idx] = std::move(tex);
	}

	void texture_pool::realloc_slots(std::uint32_t new_count)
	{
		slots = t_realloc<memory_slot>(slots, new_count);
		texture_slots = t_realloc<texture>(texture_slots, new_count);
	}

	void texture_pool::move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count)
	{
		std::memmove(&slots[dst], &slots[src], sizeof(memory_slot) * count);
		std::memmove(&texture_slots[dst], &texture_slots[src], sizeof(texture) * count);
	}

	struct dynamic_texture
	{
		VkImage image;
		std::array<VkImageView, max_frames_in_flight> views;
	};

	dynamic_texture create_dynamic_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements* out_memory_requirements)
	{
		dynamic_texture res = {};
		
		VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &res.image), "Failed to create image");

		vkGetImageMemoryRequirements(device, res.image, out_memory_requirements);

		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.flags = 0;
		view_info.image = res.image;
		view_info.format = image_info.format;

		//setting all components to identity so no swizzling occurs
		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		INTERNAL_ASSERT(image_info.arrayLayers % max_frames_in_flight == 0,
			"Number of layers indivisible by number of frames in flight");
		const std::uint32_t frame_layers = image_info.arrayLayers / max_frames_in_flight;

		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = image_info.mipLevels;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = frame_layers;

		const bool is_cube = image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		INTERNAL_ASSERT(!is_cube || image_info.imageType == VK_IMAGE_TYPE_2D, "Image type must be 2D if image is a cube");
		INTERNAL_ASSERT(!is_cube || frame_layers % 6 == 0,
			"Number of image layers must be a multiple of 6 if image is a cube");

		const bool is_array = is_cube ? 1 < frame_layers / 6 : 1 < frame_layers;

		switch (image_info.imageType)
		{
		case VK_IMAGE_TYPE_1D:
			view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
			break;
		case VK_IMAGE_TYPE_2D:
			if (is_cube) view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
			else view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			break;
		default:
			INTERNAL_ASSERT(false, "Invalid image type");
			break;
		}

		for(std::uint8_t i = 0; i < max_frames_in_flight; i++)
			VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &res.views[i]), "Failed to create image view");

		return res;
	}
}
