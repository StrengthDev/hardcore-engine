#include <pch.hpp>

#include <render/memory.hpp>
#include <render/device.hpp>

#include <debug/log_internal.hpp>

//NOTE: with the way the system is implemented, having 8MB pools assumes vertexes will have no more than 128 bytes of data

const VkDeviceSize staging_buffer_size = MEGABYTES(8);

const VkDeviceSize texture_pool_size = MEGABYTES(128);

const uint32_t initial_pool_slot_size = 16;

const VkBufferUsageFlags upload_buffer = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
const VkBufferUsageFlags compute_staging = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
const VkBufferUsageFlags download_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

const VkMemoryPropertyFlags main_heap = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
const VkMemoryPropertyFlags host_visible_heap = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags upload_heap = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags download_heap = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;


namespace ENGINE_NAMESPACE
{

	template<device_memory::buffer_t BType>
	struct buffer { static_assert(BType < device_memory::buffer_t::NONE, "Unimplemented buffer type"); };
	template<>
	struct buffer<device_memory::buffer_t::VERTEX>
	{
		static constexpr const char* debug_name = "VERTEX";
		static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		static const VkDeviceSize size = MEGABYTES(8);
		static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		static const VkDeviceSize dynamic_size = MEGABYTES(8);
	};
	template<>
	struct buffer<device_memory::buffer_t::INDEX>
	{
		static constexpr const char* debug_name = "INDEX";
		static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		static const VkDeviceSize size = MEGABYTES(8);
		static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		static const VkDeviceSize dynamic_size = MEGABYTES(8);
	};
	template<>
	struct buffer<device_memory::buffer_t::UNIFORM>
	{
		static constexpr const char* debug_name = "UNIFORM";
		static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		static const VkDeviceSize size = MEGABYTES(8);
		static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		static const VkDeviceSize dynamic_size = MEGABYTES(8);
	};
	template<>
	struct buffer<device_memory::buffer_t::STORAGE>
	{
		static constexpr const char* debug_name = "STORAGE";
		static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		static const VkDeviceSize size = MEGABYTES(8);
		static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		static const VkDeviceSize dynamic_size = MEGABYTES(8);
	};

	template<VkMemoryPropertyFlags MFlags>
	struct heap { };
	template<>
	struct heap<main_heap> { using pool_t = memory_pool; };
	template<>
	struct heap<host_visible_heap> { using pool_t = dynamic_memory_pool; };
	template<>
	struct heap<upload_heap> { using pool_t = dynamic_memory_pool; };
	template<>
	struct heap<download_heap> { using pool_t = memory_pool; };

	class mem_ref_internal : protected memory_ref
	{
		mem_ref_internal(std::uint8_t pool_type, std::uint32_t pool, std::size_t offset, std::size_t size) : 
			memory_ref(pool_type, pool, offset, size) { }

		friend device_memory;
	};

	class mesh_internal : protected mesh
	{
		friend device_memory;
	};

	class resource_internal : protected resource
	{
		friend device_memory;
	};

	memory_pool::memory_pool(VkDevice& handle, VkDeviceSize size) : size(size)
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

	void memory_pool::free(VkDevice& handle)
	{
		if (slots)
		{
			vkDestroyBuffer(handle, buffer, nullptr);
			vkFreeMemory(handle, memory, nullptr);
			std::free(slots);
			slots = nullptr;
		}
	}

	bool memory_pool::search(const VkDeviceSize size, std::uint32_t* out_slot_idx)
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
					if (size <= slots[slot_idx].size && smallest_fit > slots[slot_idx].size)
					{
						smallest_fit = slots[slot_idx].size;
						smallest_idx = slot_idx;
					}
				}
			}
			if (size <= smallest_fit)
			{
				*out_slot_idx = smallest_idx;
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
				if (!slots[slot_idx].in_use && size <= slots[slot_idx].size && smallest_fit > slots[slot_idx].size)
				{
					smallest_fit = slots[slot_idx].size;
					smallest_idx = slot_idx;
				}
			}
			*out_slot_idx = smallest_idx;
			return true;
		}
		return false;
	}

	void memory_pool::fill_slot(const std::uint32_t slot_idx, const VkDeviceSize size)
	{
		INTERNAL_ASSERT(slot_idx < n_slots, "Slot index out of bounds");
		INTERNAL_ASSERT(slot_idx == 0 ? true : slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use.");

		if (size < slots[slot_idx].size)
		{
			const std::uint32_t old_size = n_slots;
			bool selected_last = slot_idx == (n_slots - 1);

			//resize needed
			if (selected_last || (slots[slot_idx + 1].in_use && slots[n_slots - 1].in_use))
			{
				n_slots += initial_pool_slot_size; //TODO check if this is the best way to increment
				slots = t_realloc<memory_slot>(slots, n_slots);
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
			if (!selected_last)
			{
				std::uint32_t last_in_use = 0;
				for (std::uint32_t i = old_size; i != std::numeric_limits<std::uint32_t>::max(); i--)
				{
					if (slots[i].in_use)
					{
						last_in_use = i;
						break;
					}
				}
				std::memmove(&slots[slot_idx + 2], &slots[slot_idx + 1], sizeof(memory_slot) * (last_in_use - slot_idx));
			}
		}

		slots[slot_idx + 1].offset -= slots[slot_idx].size - size;
		slots[slot_idx + 1].size += slots[slot_idx].size - size;
		slots[slot_idx].in_use = true;
		slots[slot_idx].size = size;
	}

	dynamic_memory_pool::~dynamic_memory_pool()
	{

	}

	void device_memory::init(device& owner)
	{
		this->owner = &owner;

		mem_granularity = owner.properties.limits.nonCoherentAtomSize;

		vkGetPhysicalDeviceMemoryProperties(owner.physical_handle, &mem_properties);

		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			LOG_INTERNAL_INFO("[RENDERER] Memory type " << i << " properties: "
				<< '(' << std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(mem_properties.memoryTypes[i].propertyFlags) << ") => "
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED " : ""));
		}

		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = owner.transfer_idx;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CRASH_CHECK(vkCreateCommandPool(owner.handle, &pool_info, nullptr, &cmd_pool), "Failed to create command pool");
		
		VkCommandBufferAllocateInfo cmd_buffer_info = {};
		cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_info.commandPool = cmd_pool;
		cmd_buffer_info.commandBufferCount = max_frames_in_flight;

		VK_CRASH_CHECK(vkAllocateCommandBuffers(owner.handle, &cmd_buffer_info, cmd_buffers), "Failed to allocate command buffers");

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			alloc_buffer(device_in[i].device, device_in[i].buffer, staging_buffer_size, 
				upload_buffer,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VK_CRASH_CHECK(vkCreateSemaphore(owner.handle, &semaphore_info, nullptr, &device_in[i].semaphore),
				"Failed to create semaphore");
			VK_CRASH_CHECK(vkCreateSemaphore(owner.handle, &semaphore_info, nullptr, &device_out[i].semaphore),
				"Failed to create semaphore");

			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VK_CRASH_CHECK(vkCreateFence(owner.handle, &fence_info, nullptr, &device_in[i].fence), "Failed to create fence");
			VK_CRASH_CHECK(vkCreateFence(owner.handle, &fence_info, nullptr, &device_out[i].fence), "Failed to create fence");

			device_in[i].offset = 0;
		}
	}

	void device_memory::terminate()
	{
		for (std::uint8_t i = 0; i < max_frames_in_flight; i++)
		{
			vp_pending_copies[i].clear();
			ip_pending_copies[i].clear();
			up_pending_copies[i].clear();
			sp_pending_copies[i].clear();
		}

		unmap_ranges(owner->current_frame);
		for (memory_pool& pool : vertex_pools) pool.free(owner->handle);
		for (memory_pool& pool : index_pools) pool.free(owner->handle);
		for (memory_pool& pool : uniform_pools) pool.free(owner->handle);
		for (memory_pool& pool : storage_pools) pool.free(owner->handle);
		for (memory_pool& pool : d_vertex_pools) pool.free(owner->handle);
		for (memory_pool& pool : d_index_pools) pool.free(owner->handle);
		for (memory_pool& pool : d_uniform_pools) pool.free(owner->handle);
		for (memory_pool& pool : d_storage_pools) pool.free(owner->handle);

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			vkDestroyBuffer(owner->handle, device_in[i].buffer, nullptr);
			vkFreeMemory(owner->handle, device_in[i].device, nullptr);
			vkDestroySemaphore(owner->handle, device_in[i].semaphore, nullptr);
			vkDestroyFence(owner->handle, device_in[i].fence, nullptr);

			//vkDestroyBuffer(owner->handle, device_out[i].buffer, nullptr);
			vkFreeMemory(owner->handle, device_out[i].device, nullptr);
			vkDestroySemaphore(owner->handle, device_out[i].semaphore, nullptr);
			vkDestroyFence(owner->handle, device_out[i].fence, nullptr);
		}

		vkDestroyCommandPool(owner->handle, cmd_pool, nullptr);
	}

	inline VkBuffer& get_staging_buffer(transfer_memory& memory, std::uint32_t idx)
	{
		//TODO: extract correct buffer from chain
		return memory.buffer;
	}

	inline void clear_transfer_memory(transfer_memory& memory)
	{
		//TODO: free chain, probably needs to be a member function
		memory.offset = 0;
	}

	void device_memory::flush_in(const std::uint8_t current_frame)
	{
		transfer_memory& memory = device_in[current_frame];
		//TODO: if not host coherent...

		vkResetCommandBuffer(cmd_buffers[current_frame], 0);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd_buffers[current_frame], &begin_info);

		if (memory.offset) //TODO: cancel command buffer when offset is 0, make this function return bool to indicate if it is necessary to wait for the semaphore
		{
			for (auto copy_details : vp_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					vertex_pools[copy_details.first.second].get_buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : ip_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					index_pools[copy_details.first.second].get_buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : up_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					uniform_pools[copy_details.first.second].get_buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : sp_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					storage_pools[copy_details.first.second].get_buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}

			clear_transfer_memory(memory);
		}

		vkEndCommandBuffer(cmd_buffers[current_frame]);

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &cmd_buffers[current_frame];
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &memory.semaphore;

		vkResetFences(owner->handle, 1, &memory.fence);
		vkQueueSubmit(owner->transfer_queue, 1, &submit_info, memory.fence);

		memory.offset = 0;
	}

	void device_memory::map_ranges(const std::uint8_t current_frame)
	{
		VK_CRASH_CHECK(vkMapMemory(owner->handle, device_in[current_frame].device, 0, VK_WHOLE_SIZE, 0, &device_in[current_frame].host),
			"Failed to map memory");

		for (dynamic_memory_pool& pool : d_vertex_pools)
		{
			VK_CRASH_CHECK(vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * current_frame, pool.get_size(), 0, &pool.host_ptr),
				"Failed to map memory");
		}
		for (dynamic_memory_pool& pool : d_index_pools)
		{
			VK_CRASH_CHECK(vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * current_frame, pool.get_size(), 0, &pool.host_ptr),
				"Failed to map memory");
		}
		for (dynamic_memory_pool& pool : d_uniform_pools)
		{
			VK_CRASH_CHECK(vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * current_frame, pool.get_size(), 0, &pool.host_ptr),
				"Failed to map memory");
		}
		for (dynamic_memory_pool& pool : d_storage_pools)
		{
			VK_CRASH_CHECK(vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * current_frame, pool.get_size(), 0, &pool.host_ptr),
				"Failed to map memory");
		}
	}

	void device_memory::unmap_ranges(const std::uint8_t current_frame)
	{
		vkUnmapMemory(owner->handle, device_in[current_frame].device);

		for (dynamic_memory_pool& pool : d_vertex_pools)
		{
			INTERNAL_ASSERT(pool.host_ptr != nullptr, "Memmory not mapped");
			vkUnmapMemory(owner->handle, pool.get_memory());
			pool.host_ptr = nullptr;
		}
		for (dynamic_memory_pool& pool : d_index_pools)
		{
			INTERNAL_ASSERT(pool.host_ptr != nullptr, "Memmory not mapped");
			vkUnmapMemory(owner->handle, pool.get_memory());
			pool.host_ptr = nullptr;
		}
		for (dynamic_memory_pool& pool : d_uniform_pools)
		{
			INTERNAL_ASSERT(pool.host_ptr != nullptr, "Memmory not mapped");
			vkUnmapMemory(owner->handle, pool.get_memory());
			pool.host_ptr = nullptr;
		}
		for (dynamic_memory_pool& pool : d_storage_pools)
		{
			INTERNAL_ASSERT(pool.host_ptr != nullptr, "Memmory not mapped");
			vkUnmapMemory(owner->handle, pool.get_memory());
			pool.host_ptr = nullptr;
		}
	}

	void device_memory::synchronize(const std::uint8_t current_frame)
	{
		vkWaitForFences(owner->handle, 1, &device_in[current_frame].fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
	}

	memory_ref device_memory::alloc_vertices(const VkDeviceSize size) { return alloc<VERTEX, main_heap>(size); }
	memory_ref device_memory::alloc_vertices(const void* data, const VkDeviceSize size) { return alloc<VERTEX, main_heap>(data, size); }
	memory_ref device_memory::alloc_indexes(const VkDeviceSize size) { return alloc<INDEX, main_heap>(size); }
	memory_ref device_memory::alloc_indexes(const void* data, const VkDeviceSize size) { return alloc<INDEX, main_heap>(data, size); }
	memory_ref device_memory::alloc_uniform(const VkDeviceSize size) { return alloc<UNIFORM, main_heap>(size); }
	memory_ref device_memory::alloc_uniform(const void* data, const VkDeviceSize size) { return alloc<UNIFORM, main_heap>(data, size); }
	memory_ref device_memory::alloc_storage(const VkDeviceSize size) { return alloc<STORAGE, main_heap>(size); }
	memory_ref device_memory::alloc_storage(const void* data, const VkDeviceSize size) { return alloc<STORAGE, main_heap>(data, size); }

	memory_ref device_memory::alloc_dynamic_vertices(const VkDeviceSize size)
	{ return alloc<VERTEX, host_visible_heap>(size); }
	memory_ref device_memory::alloc_dynamic_vertices(const void* data, const VkDeviceSize size)
	{ return alloc<VERTEX, host_visible_heap>(data, size); }
	memory_ref device_memory::alloc_dynamic_indexes(const VkDeviceSize size)
	{ return alloc<INDEX, host_visible_heap>(size); }
	memory_ref device_memory::alloc_dynamic_indexes(const void* data, const VkDeviceSize size)
	{ return alloc<INDEX, host_visible_heap>(data, size); }
	memory_ref device_memory::alloc_dynamic_uniform(const VkDeviceSize size)
	{ return alloc<UNIFORM, host_visible_heap>(size); }
	memory_ref device_memory::alloc_dynamic_uniform(const void* data, const VkDeviceSize size)
	{ return alloc<UNIFORM, host_visible_heap>(data, size); }
	memory_ref device_memory::alloc_dynamic_storage(const VkDeviceSize size)
	{ return alloc<STORAGE, host_visible_heap>(size); }
	memory_ref device_memory::alloc_dynamic_storage(const void* data, const VkDeviceSize size)
	{ return alloc<STORAGE, host_visible_heap>(data, size); }

	void device_memory::submit_upload(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref); //TODO seperate into different functions like memcpy
		INTERNAL_ASSERT(offset + size <= int_ref.size, "Out of bounds memory access");
		switch (int_ref.pool_type)
		{
		case VERTEX: submit_upload<VERTEX>(int_ref.pool, int_ref.offset + offset, data, size); break;
		case INDEX: submit_upload<INDEX>(int_ref.pool, int_ref.offset + offset, data, size); break;
		case UNIFORM: submit_upload<UNIFORM>(int_ref.pool, int_ref.offset + offset, data, size); break;
		case STORAGE: submit_upload<STORAGE>(int_ref.pool, int_ref.offset + offset, data, size); break;
		default:
			INTERNAL_ASSERT(false, "Unimplemented buffer type");
			break;
		}
	}

	void device_memory::memcpy_vertices(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{ this->memcpy<VERTEX>(ref, data, size, offset); }
	void device_memory::memcpy_indexes(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{ this->memcpy<INDEX>(ref, data, size, offset); }
	void device_memory::memcpy_uniform(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{ this->memcpy<UNIFORM>(ref, data, size, offset); }
	void device_memory::memcpy_storage(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{ this->memcpy<STORAGE>(ref, data, size, offset); }

	void device_memory::get_vertex_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ get_map<VERTEX>(ref, out_map_ptr, out_offset); }
	void device_memory::get_index_map(const memory_ref & ref, void*** out_map_ptr, std::size_t * out_offset) noexcept
	{ get_map<INDEX>(ref, out_map_ptr, out_offset); }
	void device_memory::get_uniform_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ get_map<UNIFORM>(ref, out_map_ptr, out_offset); }
	void device_memory::get_storage_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ get_map<STORAGE>(ref, out_map_ptr, out_offset); }

	buffer_binding_args device_memory::get_binding_args(const mesh& object) noexcept
	{ return get_binding_args<VERTEX>(object); }
	buffer_binding_args device_memory::get_binding_args(const uniform& uniform) noexcept
	{ return get_dynamic_binding_args<UNIFORM>(uniform); }
	buffer_binding_args device_memory::get_binding_args(const unmapped_uniform& uniform) noexcept
	{ return get_binding_args<UNIFORM>(uniform); }
	buffer_binding_args device_memory::get_binding_args(const storage_array& array) noexcept
	{ return get_binding_args<STORAGE>(array); }
	buffer_binding_args device_memory::get_binding_args(const dynamic_storage_array& array) noexcept
	{ return get_dynamic_binding_args<STORAGE>(array); }
	buffer_binding_args device_memory::get_binding_args(const storage_vector& vector) noexcept
	{ return get_binding_args<STORAGE>(vector); }
	buffer_binding_args device_memory::get_binding_args(const dynamic_storage_vector& vector) noexcept
	{ return get_dynamic_binding_args<STORAGE>(vector); }

	std::uint32_t device_memory::find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		//Exact type search
		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && mem_properties.memoryTypes[i].propertyFlags == properties)
			{
				return i;
			}
		}
		LOG_INTERNAL_WARN("Failed to find exact memory type.");

		//Relaxed search
		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		//not sure how to deal with this
		CRASH("Could not find suitable memory type");
		LOG_INTERNAL_ERROR("Failed to find suitable memory type. Type bit mask: "
			<< std::bitset<sizeof(type_filter) * 8>(type_filter) << " Property flags: "
			<< std::bitset<sizeof(properties) * 8>(properties));
		return std::numeric_limits<uint32_t>::max();
	}

	void device_memory::alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, 
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CRASH_CHECK(vkCreateBuffer(owner->handle, &buffer_info, nullptr, &buffer), "Failed to create buffer");

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(owner->handle, buffer, &memory_requirements);

		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = memory_requirements.size;
		memory_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

		VK_CRASH_CHECK(vkAllocateMemory(owner->handle, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		vkBindBufferMemory(owner->handle, buffer, memory, 0);
	}

	template<typename Pool>
	inline bool search_pools(std::vector<Pool>& pools, const VkDeviceSize size,
		std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx)
	{
		static_assert(std::is_same<Pool, memory_pool>::value || std::is_same<Pool, dynamic_memory_pool>::value, "Invalid pool type");

		std::uint32_t selected_pool_idx = 0;
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		for (memory_pool& pool : pools)
		{
			if (pool.search(size, &selected_slot_idx)) break;
			selected_pool_idx++;
		}
		*out_pool_idx = selected_pool_idx;
		if (selected_slot_idx == std::numeric_limits<std::uint32_t>::max())
		{
			*out_slot_idx = 0;
			return false;
		}
		else
		{
			*out_slot_idx = selected_slot_idx;
			return true;
		}
	}
	
	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline void device_memory::alloc_slot(std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;
		std::vector<pool_t>* pools;
		VkDeviceSize pool_size;
		if constexpr (std::is_same<pool_t, memory_pool>::value)
		{
			pools = &get_pools<BType>();
			pool_size = buffer<BType>::size;
		}
		else
		{
			pools = &get_dynamic_pools<BType>();
			pool_size = buffer<BType>::dynamic_size;
		}

		std::uint32_t selected_pool_idx = 0;
		std::uint32_t selected_slot_idx = 0;

		if (!search_pools(*pools, size, &selected_pool_idx, &selected_slot_idx))
		{
			if (pool_size < size)
			{
				//TODO increase pool_size
			}

			if constexpr (std::is_same<pool_t, memory_pool>::value)
			{
				pools->push_back(memory_pool(owner->handle, pool_size));
				alloc_buffer(pools->back().get_memory(), pools->back().get_buffer(), pool_size, buffer<BType>::usage, MFlags);
			}
			else
			{
				pools->push_back(dynamic_memory_pool(owner->handle, pool_size));
				alloc_buffer(pools->back().get_memory(), pools->back().get_buffer(), pool_size * max_frames_in_flight,
					buffer<BType>::dynamic_usage, MFlags);
			}
		}

		(*pools)[selected_pool_idx].fill_slot(selected_slot_idx, size);
		*out_pool_idx = selected_pool_idx;
		*out_slot_idx = selected_slot_idx;
		LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} of pool {2} of type {3}", size, selected_slot_idx, selected_pool_idx, 
			buffer<BType>::debug_name);
	}

	//TODO change or add version that takes memory ref as argument
	template<device_memory::buffer_t BType>
	inline void device_memory::submit_upload(std::uint32_t pool_idx, VkDeviceSize offset, const void* data, const VkDeviceSize size)
	{
		transfer_memory* memory_ptr = &device_in[owner->current_frame];
		std::uint32_t transfer_idx = 0;
		if (memory_ptr->offset + size > staging_buffer_size)
		{
			//TODO: add new tranfer memory to "memory", then update "memory_ptr" and "transfer_idx"
			//call submit_upload recursively, copy large blocks of memory in chunks
		}

		memory_pool& pool = get_pools<BType>()[pool_idx];
		VkBufferCopy info = {};
		info.srcOffset = memory_ptr->offset;
		info.dstOffset = offset;
		info.size = size;

		std::memcpy(reinterpret_cast<char*>(memory_ptr->host) + memory_ptr->offset, data, size);

		get_pending_copies<BType>(owner->current_frame)[copy_path_t(transfer_idx, pool_idx)].push_back(info);
		memory_ptr->offset += size;
	}

	template<device_memory::buffer_t BType>
	inline void device_memory::memcpy(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref);
		std::vector<dynamic_memory_pool>& pools = get_dynamic_pools<BType>();
		INTERNAL_ASSERT(int_ref.offset + offset + size <= int_ref.size, "Out of bounds memory access");
		std::memcpy(reinterpret_cast<std::byte*>(pools[int_ref.pool].host_ptr) + int_ref.offset + offset, data, size);
	}

	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline memory_ref device_memory::alloc(const VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;

		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();

		alloc_slot<BType, MFlags>(&selected_pool_idx, &selected_slot_idx, size);
		if constexpr (std::is_same<pool_t, memory_pool>::value)
		{
			return mem_ref_internal(BType, selected_pool_idx, 
				get_pools<BType>()[selected_pool_idx].get_slot(selected_slot_idx).offset, size);
		}
		else
		{
			dynamic_memory_pool& pool = get_dynamic_pools<BType>()[selected_pool_idx];
			vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * owner->current_frame, pool.get_size(), 0, &pool.host_ptr);
			return mem_ref_internal(BType, selected_pool_idx, pool.get_slot(selected_slot_idx).offset, size);
		}
	}

	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline memory_ref device_memory::alloc(const void* data, const VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;

		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();

		alloc_slot<BType, MFlags>(&selected_pool_idx, &selected_slot_idx, size);
		if constexpr (std::is_same<pool_t, memory_pool>::value)
		{
			const memory_slot& slot = get_pools<BType>()[selected_pool_idx].get_slot(selected_slot_idx);
			submit_upload<BType>(selected_pool_idx, slot.offset, data, size);
			return mem_ref_internal(BType, selected_pool_idx, slot.offset, size);
		}
		else
		{
			dynamic_memory_pool& pool = get_dynamic_pools<BType>()[selected_pool_idx];
			vkMapMemory(owner->handle, pool.get_memory(), pool.get_size() * owner->current_frame, pool.get_size(), 0, &pool.host_ptr);
			mem_ref_internal ref(BType, selected_pool_idx, pool.get_slot(selected_slot_idx).offset, size);
			this->memcpy<BType>(ref, data, size, 0);
			return std::move(ref);
		}
	}

	template<device_memory::buffer_t BType>
	void device_memory::get_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref);
		*out_offset = int_ref.offset;
		*out_map_ptr = &get_dynamic_pools<BType>()[int_ref.pool].host_ptr;
	}

	buffer_binding_args device_memory::get_index_binding_args(const mesh& object) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const mesh_internal&>(object).index_ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		return { index_pools[ref.pool].get_buffer(), index_pools[ref.pool].get_size(), ref.offset, ref.size };
	}

	template<device_memory::buffer_t BType>
	buffer_binding_args device_memory::get_binding_args(const resource& resource) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const resource_internal&>(resource).ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		//memory_pool& pool = get_pools<BType>()[ref.pool];
		return { get_pools<BType>()[ref.pool].get_buffer(), 0, ref.offset, ref.size };
	}

	template<device_memory::buffer_t BType>
	buffer_binding_args device_memory::get_dynamic_binding_args(const resource& resource) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const resource_internal&>(resource).ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		dynamic_memory_pool& pool = get_dynamic_pools<BType>()[ref.pool];
		return { pool.get_buffer(), pool.get_size(), ref.offset, ref.size };
	}
}
