#include <pch.hpp>

#include <render/memory_pool.hpp>

namespace ENGINE_NAMESPACE
{
	const std::uint32_t initial_pool_slot_size = 16;

	memory_pool::memory_pool(VkDevice device, VkDeviceSize size) : _size(size)
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
	}

	memory_pool::~memory_pool()
	{
		INTERNAL_ASSERT(!slots, "Memory pool not freed");
	}

	void memory_pool::free(VkDevice device)
	{
		if (slots)
		{
			vkFreeMemory(device, _memory, nullptr);
			std::free(slots);
			slots = nullptr;
		}
	}

	bool memory_pool::search(VkDeviceSize size, VkDeviceSize alignment, 
		std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed)
	{
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

	void buffer_pool::free(VkDevice device)
	{
		if (slots)
		{
			vkDestroyBuffer(device, _buffer, nullptr);
			memory_pool::free(device);
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

	void texture_pool::realloc_slots(std::uint32_t new_count)
	{
		slots = t_realloc<memory_slot>(slots, new_count);
		texture_slots = t_realloc<texture_slot>(texture_slots, new_count);
	}

	void texture_pool::move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count)
	{
		std::memmove(&slots[dst], &slots[src], sizeof(memory_slot) * count);
		std::memmove(&texture_slots[dst], &texture_slots[src], sizeof(texture_slot) * count);
	}

}
