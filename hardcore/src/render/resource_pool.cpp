#include <pch.hpp>

#include <render/resource_pool.hpp>

namespace ENGINE_NAMESPACE
{
	const u32 initial_pool_slot_size = 16;
	
	memory_pool::memory_pool(VkDeviceSize size, bool per_frame_allocation) : m_size(size)
	{
		n_slots = initial_pool_slot_size;
		slots = t_malloc<memory_slot>(initial_pool_slot_size);
		slots[0].in_use = false;
		slots[0].offset = 0;
		slots[0].size = size;
		m_largest_free_slot = 0;

		for (u32 i = 1; i < n_slots; i++)
		{
			slots[i].in_use = false;
			slots[i].offset = size;
			slots[i].size = 0;
		}

		this->m_per_frame_allocation = per_frame_allocation;
	}

	memory_pool::~memory_pool()
	{
		INTERNAL_ASSERT(!slots, "Resource pool not freed");
	}

	void memory_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		if (slots)
		{
			heap_manager.free(device, m_memory);
			std::free(slots);
			slots = nullptr;
		}
	}

	//TODO could try making allocations more efficient my reducing the size of the alignment padding
	bool memory_pool::search(VkDeviceSize size, VkDeviceSize alignment, 
		u32& out_slot_idx, VkDeviceSize& out_size_needed, VkDeviceSize& out_offset) const
	{
		if (m_size < size)
			return false;

		constexpr u32 invalid_idx = std::numeric_limits<u32>::max();

		// largest free slot is unknown, must check everything, look for smallest possible fit
		// if possible, update largest free slot to reduce cost of future calls
		if (slots[m_largest_free_slot].in_use)
		{
			VkDeviceSize largest_fit = 0;
			u32 largest_idx = invalid_idx;
			VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
			u32 smallest_idx = invalid_idx;
			VkDeviceSize smallest_offset = 0;
			
			VkDeviceSize offset = 0;
			for (u32 i = 0; i < n_slots; i++)
			{
				offset += slots[i].offset;

				if (!slots[i].in_use)
				{
					if (largest_fit < slots[i].size)
					{
						largest_fit = slots[i].size;
						largest_idx = i;
					}
					if (smallest_fit > slots[i].size && size + alignment_pad(slots[i].offset, alignment) <= slots[i].size)
					{
						smallest_fit = slots[i].size;
						smallest_idx = i;
						smallest_offset = offset;
					}
				}
			}
			
			if (smallest_idx != invalid_idx)
			{
				if (largest_idx != invalid_idx && largest_idx != smallest_idx)
					m_largest_free_slot = largest_idx;

				out_slot_idx = smallest_idx;
				out_size_needed = size + alignment_pad(slots[smallest_idx].offset, alignment);
				out_offset = smallest_offset;
				return true;
			}
			else
			{
				if (largest_idx != invalid_idx)
					m_largest_free_slot = largest_idx;

				return false;
			}
		}

		// largest free slot is known, so if it is large enough, assign a slot in this pool, 
		// otherwise move on to next pool
		else
		{
			if (slots[m_largest_free_slot].size < size)
				return false;

			VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
			u32 smallest_idx = invalid_idx;
			VkDeviceSize smallest_offset = 0;

			VkDeviceSize offset = 0;
			for (u32 i = 0; i < n_slots; i++)
			{
				offset += slots[i].offset;

				if (!slots[i].in_use 
					&& smallest_fit > slots[i].size
					&& size + alignment_pad(slots[i].offset, alignment) <= slots[i].size)
				{
					smallest_fit = slots[i].size;
					smallest_idx = i;
					smallest_offset = offset;
				}
			}

			out_slot_idx = smallest_idx;
			out_size_needed = size + alignment_pad(slots[smallest_idx].offset, alignment);
			out_offset = smallest_offset;
			return true;
		}

		return false;
	}

	void memory_pool::fill_slot(u32 slot_idx, VkDeviceSize size)
	{
		INTERNAL_ASSERT(slot_idx < n_slots, "Slot index out of bounds");
		INTERNAL_ASSERT(size <= slots[slot_idx].size, "Allocation size greater than slot size");
		INTERNAL_ASSERT(slot_idx == 0 ? true : slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use.");

		if (size < slots[slot_idx].size)
		{
			const u32 old_size = n_slots;
			bool selected_last = slot_idx == (n_slots - 1);

			//resize needed
			if (selected_last || (slots[slot_idx + 1].in_use && slots[n_slots - 1].in_use))
			{
				n_slots += initial_pool_slot_size; //TODO check if this is the best way to increment
				realloc_slots(n_slots);
				for (u32 i = old_size; i < n_slots; i++)
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
				u32 last_in_use = 0;
				for (u32 i = old_size - 1; i > slot_idx; i--)
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

	u32 memory_pool::find_slot(VkDeviceSize offset) const noexcept
	{
		VkDeviceSize total_offset = 0;
		u32 i = 0;
		for (; i < n_slots && total_offset < offset; i++)
			total_offset += slots[i].offset;

		if (total_offset == offset)
			return i;
		else
			return std::numeric_limits<u32>::max();
	}

	buffer_pool::buffer_pool(VkDevice device, device_heap_manager& heap_manager,
		VkDeviceSize size, VkBufferUsageFlags usage, device_heap_manager::heap heap, bool per_frame_allocation) :
		memory_pool(size, per_frame_allocation)
	{
		heap_manager.alloc_buffer(device, m_memory, m_buffer, 
			per_frame_allocation ? size * max_frames_in_flight : size, 
			usage, heap);
	}

	void buffer_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		if (slots)
		{
			vkDestroyBuffer(device, m_buffer, nullptr);
			memory_pool::free(device, heap_manager);
		}
	}

	void buffer_pool::realloc_slots(u32 new_count)
	{
		slots = t_realloc<memory_slot>(slots, new_count);
	}

	void buffer_pool::move_slots(u32 dst, u32 src, u32 count)
	{
		std::memmove(&slots[dst], &slots[src], sizeof(*slots) * count);
	}

	void dynamic_buffer_pool::map(VkDevice device, u8 current_frame)
	{
		VK_CRASH_CHECK(vkMapMemory(device, m_memory, m_size * current_frame, m_size, 0, &m_host_ptr), 
			"Failed to map memory");
	}

	void dynamic_buffer_pool::unmap(VkDevice device)
	{
		INTERNAL_ASSERT(m_host_ptr != nullptr, "Memmory not mapped");
		vkUnmapMemory(device, m_memory);
		m_host_ptr = nullptr;
	}

	void dynamic_buffer_pool::push_flush_ranges(std::vector<VkMappedMemoryRange>& ranges, u8 current_frame,
		const std::vector<dynamic_buffer_pool>& pools)
	{
		for (const dynamic_buffer_pool& pool : pools)
		{
			//TODO skip pools with no allocations, need to track allocations somehow

			ranges.push_back(pool.mapped_range(current_frame));
		}
	}

	texture_slot create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements& out_memory_requirements)
	{
		VkImage image;
		VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &image), "Failed to create image");

		vkGetImageMemoryRequirements(device, image, &out_memory_requirements);

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
		INTERNAL_ASSERT(!is_array || (is_array && image_info.imageType != VK_IMAGE_TYPE_3D), 
			"3D image arrays are invalid");

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

		//VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &image_view), "Failed to create image view");

		return {
			.image = image,
			.view = VK_NULL_HANDLE,// image_view,
			.size = out_memory_requirements.size,
			.dims = image_info.extent,
			.layout = image_info.initialLayout 
		};
	}

	texture_pool::texture_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size,
		u32 memory_type_bits, device_heap_manager::heap preferred_heap) :
		memory_pool(size)
	{
		memory_type_idx = heap_manager.alloc_texture_memory(device, m_memory, size, preferred_heap, memory_type_bits);
		texture_slots = t_malloc<texture_slot>(initial_pool_slot_size);
	}

	void texture_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		memory_pool::free(device, heap_manager);
		std::free(texture_slots);
	}

	bool texture_pool::search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
		u32& out_slot_idx, VkDeviceSize& out_size_needed, VkDeviceSize& out_offset) const
	{
		if (memory_type_bits & (1ULL << memory_type_idx))
			return memory_pool::search(size, alignment, out_slot_idx, out_size_needed, out_offset);

		return false;
	}

	void texture_pool::fill_slot(VkDevice device, texture_slot&& tex, u32 slot_idx, VkDeviceSize size, VkDeviceSize alignment)
	{
		memory_pool::fill_slot(slot_idx, size);

		// memory_pool::fill_slot should perform any reallocation if necessary, so the slot should always exist

		texture_slots[slot_idx] = std::move(tex);

		VK_CRASH_CHECK(vkBindImageMemory(device, texture_slots[slot_idx].image, m_memory,
			aligned_offset(slots[slot_idx].offset, alignment)), "Failed to bind image to memory");
	}

	void texture_pool::realloc_slots(u32 new_count)
	{
		slots = t_realloc<memory_slot>(slots, new_count);
		texture_slots = t_realloc<texture_slot>(texture_slots, new_count);
	}

	void texture_pool::move_slots(u32 dst, u32 src, u32 count)
	{
		std::memmove(&slots[dst], &slots[src], sizeof(*slots) * count);
		std::memmove(&texture_slots[dst], &texture_slots[src], sizeof(*texture_slots) * count);
	}

	struct dynamic_texture
	{
		VkImage image;
		VkImageLayout layout;
		std::array<VkImageView, max_frames_in_flight> views;
	};

	dynamic_texture create_dynamic_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements& out_memory_requirements)
	{
		dynamic_texture res = {};
		
		VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &res.image), "Failed to create image");

		vkGetImageMemoryRequirements(device, res.image, &out_memory_requirements);

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
		const u32 frame_layers = image_info.arrayLayers / max_frames_in_flight;

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

		for(u8 i = 0; i < max_frames_in_flight; i++)
			VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &res.views[i]), "Failed to create image view");

		return res;
	}
}
