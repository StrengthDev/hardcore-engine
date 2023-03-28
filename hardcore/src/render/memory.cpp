#include <pch.hpp>

#include <render/memory.hpp>
#include <render/device.hpp>

#include <debug/log_internal.hpp>

//NOTE: with the way the system is implemented, having 8MB pools assumes vertexes will have no more than 128 bytes of data

const VkDeviceSize staging_buffer_size = MEGABYTES(8);

const VkDeviceSize texture_pool_size = MEGABYTES(128);

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
	struct heap<main_heap> { using pool_t = buffer_pool; };
	template<>
	struct heap<host_visible_heap> { using pool_t = dynamic_buffer_pool; };
	template<>
	struct heap<upload_heap> { using pool_t = dynamic_buffer_pool; };
	template<>
	struct heap<download_heap> { using pool_t = buffer_pool; };

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

	template<typename Pool>
	inline bool search_pools(std::vector<Pool>& pools, const VkDeviceSize size, VkDeviceSize alignment,
		std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed)
	{
		static_assert(std::is_base_of<memory_pool, Pool>::value, "Invalid pool type");

		std::uint32_t selected_pool_idx = 0;
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		for (memory_pool& pool : pools)
		{
			if (pool.search(size, alignment, &selected_slot_idx, out_size_needed)) break;
			selected_pool_idx++;
		}
		*out_pool_idx = selected_pool_idx;
		if (selected_slot_idx == std::numeric_limits<std::uint32_t>::max())
		{
			*out_slot_idx = 0;
			*out_size_needed = size;
			return false;
		}
		else
		{
			*out_slot_idx = selected_slot_idx;
			return true;
		}
	}

	/* 
	+---------------+
	| Device Memory |
	+---------------+
	*/

	void device_memory::init(VkPhysicalDevice physical_device, VkDevice device, std::uint32_t transfer_queue_idx,
		const VkPhysicalDeviceLimits* limits, const std::uint8_t* current_frame)
	{
		update_refs(device, limits, current_frame);

		vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

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
		pool_info.queueFamilyIndex = transfer_queue_idx;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CRASH_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &cmd_pool), "Failed to create command pool");
		
		VkCommandBufferAllocateInfo cmd_buffer_info = {};
		cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_info.commandPool = cmd_pool;
		cmd_buffer_info.commandBufferCount = max_frames_in_flight;

		VK_CRASH_CHECK(vkAllocateCommandBuffers(device, &cmd_buffer_info, cmd_buffers.data()), "Failed to allocate command buffers");

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			alloc_buffer(device_in[i].device, device_in[i].buffer, staging_buffer_size, 
				upload_buffer,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VK_CRASH_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &device_in[i].semaphore),
				"Failed to create semaphore");
			VK_CRASH_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &device_out[i].semaphore),
				"Failed to create semaphore");

			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VK_CRASH_CHECK(vkCreateFence(device, &fence_info, nullptr, &device_in[i].fence), "Failed to create fence");
			VK_CRASH_CHECK(vkCreateFence(device, &fence_info, nullptr, &device_out[i].fence), "Failed to create fence");

			device_in[i].offset = 0;
		}
	}

	void device_memory::terminate(VkDevice device, std::uint8_t current_frame)
	{
		for (std::uint8_t i = 0; i < max_frames_in_flight; i++)
		{
			vp_pending_copies[i].clear();
			ip_pending_copies[i].clear();
			up_pending_copies[i].clear();
			sp_pending_copies[i].clear();
		}

		unmap_ranges(device, current_frame);
		for (buffer_pool& pool : vertex_pools) pool.free(device);
		vertex_pools.clear();
		for (buffer_pool& pool : index_pools) pool.free(device);
		index_pools.clear();
		for (buffer_pool& pool : uniform_pools) pool.free(device);
		uniform_pools.clear();
		for (buffer_pool& pool : storage_pools) pool.free(device);
		storage_pools.clear();
		for (buffer_pool& pool : d_vertex_pools) pool.free(device);
		d_vertex_pools.clear();
		for (buffer_pool& pool : d_index_pools) pool.free(device);
		d_index_pools.clear();
		for (buffer_pool& pool : d_uniform_pools) pool.free(device);
		d_uniform_pools.clear();
		for (buffer_pool& pool : d_storage_pools) pool.free(device);
		d_storage_pools.clear();

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			vkDestroyBuffer(device, device_in[i].buffer, nullptr);
			vkFreeMemory(device, device_in[i].device, nullptr);
			vkDestroySemaphore(device, device_in[i].semaphore, nullptr);
			vkDestroyFence(device, device_in[i].fence, nullptr);

			//vkDestroyBuffer(device, device_out[i].buffer, nullptr);
			//vkFreeMemory(device, device_out[i].device, nullptr);
			vkDestroySemaphore(device, device_out[i].semaphore, nullptr);
			vkDestroyFence(device, device_out[i].fence, nullptr);
		}

		vkDestroyCommandPool(device, cmd_pool, nullptr);
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

	void device_memory::flush_in(VkDevice device, VkQueue transfer_queue, std::uint8_t current_frame)
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
					vertex_pools[copy_details.first.second].buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : ip_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					index_pools[copy_details.first.second].buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : up_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					uniform_pools[copy_details.first.second].buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
					copy_details.second.data());
			}
			for (auto copy_details : sp_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, copy_details.first.first),
					storage_pools[copy_details.first.second].buffer(), static_cast<std::uint32_t>(copy_details.second.size()),
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

		vkResetFences(device, 1, &memory.fence);
		vkQueueSubmit(transfer_queue, 1, &submit_info, memory.fence);

		memory.offset = 0;
	}

	void device_memory::map_ranges(VkDevice device, std::uint8_t current_frame)
	{
		VK_CRASH_CHECK(vkMapMemory(device, device_in[current_frame].device, 0, VK_WHOLE_SIZE, 0, &device_in[current_frame].host),
			"Failed to map memory");

		for (dynamic_buffer_pool& pool : d_vertex_pools)	pool.map(device, current_frame);
		for (dynamic_buffer_pool& pool : d_index_pools)		pool.map(device, current_frame);
		for (dynamic_buffer_pool& pool : d_uniform_pools)	pool.map(device, current_frame);
		for (dynamic_buffer_pool& pool : d_storage_pools)	pool.map(device, current_frame);
	}

	void device_memory::unmap_ranges(VkDevice device, std::uint8_t current_frame)
	{
		vkUnmapMemory(device, device_in[current_frame].device);

		for (dynamic_buffer_pool& pool : d_vertex_pools)	pool.unmap(device);
		for (dynamic_buffer_pool& pool : d_index_pools)		pool.unmap(device);
		for (dynamic_buffer_pool& pool : d_uniform_pools)	pool.unmap(device);
		for (dynamic_buffer_pool& pool : d_storage_pools)	pool.unmap(device);
	}

	void device_memory::synchronize(VkDevice device, std::uint8_t current_frame)
	{
		vkWaitForFences(device, 1, &device_in[current_frame].fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
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

	void device_memory::vertex_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ map<VERTEX>(ref, out_map_ptr, out_offset); }
	void device_memory::index_map(const memory_ref & ref, void*** out_map_ptr, std::size_t * out_offset) noexcept
	{ map<INDEX>(ref, out_map_ptr, out_offset); }
	void device_memory::uniform_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ map<UNIFORM>(ref, out_map_ptr, out_offset); }
	void device_memory::storage_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{ map<STORAGE>(ref, out_map_ptr, out_offset); }

	buffer_binding_args device_memory::binding_args(const mesh& object) noexcept
	{ return binding_args<VERTEX>(object); }
	buffer_binding_args device_memory::binding_args(const uniform& uniform) noexcept
	{ return dynamic_binding_args<UNIFORM>(uniform); }
	buffer_binding_args device_memory::binding_args(const unmapped_uniform& uniform) noexcept
	{ return binding_args<UNIFORM>(uniform); }
	buffer_binding_args device_memory::binding_args(const storage_array& array) noexcept
	{ return binding_args<STORAGE>(array); }
	buffer_binding_args device_memory::binding_args(const dynamic_storage_array& array) noexcept
	{ return dynamic_binding_args<STORAGE>(array); }
	buffer_binding_args device_memory::binding_args(const storage_vector& vector) noexcept
	{ return binding_args<STORAGE>(vector); }
	buffer_binding_args device_memory::binding_args(const dynamic_storage_vector& vector) noexcept
	{ return dynamic_binding_args<STORAGE>(vector); }

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
		return std::numeric_limits<std::uint32_t>::max();
	}

	void device_memory::alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, 
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CRASH_CHECK(vkCreateBuffer(radd.device, &buffer_info, nullptr, &buffer), "Failed to create buffer");

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(radd.device, buffer, &memory_requirements);

		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = memory_requirements.size;
		memory_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

		VK_CRASH_CHECK(vkAllocateMemory(radd.device, &memory_info, nullptr, &memory), "Failed to allocate device memory");

		vkBindBufferMemory(radd.device, buffer, memory, 0);
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

		VkMemoryRequirements memory_requirements;
		//vkGetImageMemoryRequirements(radd.device, image, &memory_requirements);

		//vkBindImageMemory(radd.device, image, memory, 0);
	}
	
	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline void device_memory::alloc_slot(std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;
		const VkDeviceSize alignment = offset_alignment<BType>();
		std::vector<pool_t>* pools;
		VkDeviceSize pool_size;
		if constexpr (std::is_same<pool_t, buffer_pool>::value)
		{
			pools = &static_pools<BType>();
			pool_size = buffer<BType>::size;
		}
		else
		{
			pools = &dynamic_pools<BType>();
			pool_size = buffer<BType>::dynamic_size;
		}

		std::uint32_t selected_pool_idx = 0;
		std::uint32_t selected_slot_idx = 0;
		VkDeviceSize size_needed = 0;

		if (!search_pools(*pools, size, alignment, &selected_pool_idx, &selected_slot_idx, &size_needed))
		{
			if (pool_size < size)
			{
				//TODO increase pool_size
			}

			if constexpr (std::is_same<pool_t, buffer_pool>::value)
			{
				pools->push_back(buffer_pool(radd.device, pool_size));
				alloc_buffer(pools->back().memory(), pools->back().buffer(), pool_size, buffer<BType>::usage, MFlags);
			}
			else
			{
				pools->push_back(dynamic_buffer_pool(radd.device, pool_size));
				alloc_buffer(pools->back().memory(), pools->back().buffer(), pool_size * max_frames_in_flight,
					buffer<BType>::dynamic_usage, MFlags);
				pool_t& pool = (*pools)[selected_pool_idx];
				pool.map(radd.device, *radd.current_frame);
			}
		}

		(*pools)[selected_pool_idx].fill_slot(selected_slot_idx, size_needed);
		*out_pool_idx = selected_pool_idx;
		*out_slot_idx = selected_slot_idx;
		LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} (offset {2}->{3}) of pool {4} {5}", 
			size, selected_slot_idx, 
			(*pools)[selected_pool_idx][selected_slot_idx].offset,
			aligned_offset((*pools)[selected_pool_idx][selected_slot_idx].offset, alignment),
			buffer<BType>::debug_name, selected_pool_idx);
	}

	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline memory_ref device_memory::alloc(const VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;

		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();

		alloc_slot<BType, MFlags>(&selected_pool_idx, &selected_slot_idx, size);
		if constexpr (std::is_same<pool_t, buffer_pool>::value)
		{
			return mem_ref_internal(BType, selected_pool_idx, 
				static_pools<BType>()[selected_pool_idx][selected_slot_idx].offset, size);
		}
		else
		{
			dynamic_buffer_pool& pool = dynamic_pools<BType>()[selected_pool_idx];
			return mem_ref_internal(BType, selected_pool_idx, pool[selected_slot_idx].offset, size);
		}
	}

	template<device_memory::buffer_t BType, VkMemoryPropertyFlags MFlags>
	inline memory_ref device_memory::alloc(const void* data, const VkDeviceSize size)
	{
		using pool_t = typename heap<MFlags>::pool_t;

		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();

		alloc_slot<BType, MFlags>(&selected_pool_idx, &selected_slot_idx, size);
		if constexpr (std::is_same<pool_t, buffer_pool>::value)
		{
			const memory_slot& slot = static_pools<BType>()[selected_pool_idx][selected_slot_idx];
			submit_upload<BType>(selected_pool_idx, slot.offset, data, size);
			return mem_ref_internal(BType, selected_pool_idx, slot.offset, size);
		}
		else
		{
			dynamic_buffer_pool& pool = dynamic_pools<BType>()[selected_pool_idx];
			mem_ref_internal ref(BType, selected_pool_idx, pool[selected_slot_idx].offset, size);
			this->memcpy<BType>(ref, data, size, 0);
			return std::move(ref);
		}
	}

	//TODO change or add version that takes memory ref as argument
	template<device_memory::buffer_t BType>
	inline void device_memory::submit_upload(std::uint32_t pool_idx, VkDeviceSize offset, const void* data, const VkDeviceSize size)
	{
		transfer_memory* memory_ptr = &device_in[*radd.current_frame];
		std::uint32_t transfer_idx = 0;
		if (memory_ptr->offset + size > staging_buffer_size)
		{
			//TODO: add new tranfer memory to "memory", then update "memory_ptr" and "transfer_idx"
			//call submit_upload recursively, copy large blocks of memory in chunks
		}

		buffer_pool& pool = static_pools<BType>()[pool_idx];
		VkBufferCopy info = {};
		info.srcOffset = memory_ptr->offset;
		info.dstOffset = offset;
		info.size = size;

		std::memcpy(reinterpret_cast<char*>(memory_ptr->host) + memory_ptr->offset, data, size);

		pending_copies<BType>(*radd.current_frame)[copy_path_t(transfer_idx, pool_idx)].push_back(info);
		memory_ptr->offset += size;
	}

	template<device_memory::buffer_t BType>
	inline void device_memory::memcpy(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref);
		std::vector<dynamic_buffer_pool>& pools = dynamic_pools<BType>();
		INTERNAL_ASSERT(int_ref.offset + offset + size <= int_ref.size, "Out of bounds memory access");
		std::memcpy(reinterpret_cast<std::byte*>(pools[int_ref.pool].host_ptr()) + int_ref.offset + offset, data, size);
	}

	template<device_memory::buffer_t BType>
	void device_memory::map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept
	{
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref);
		*out_offset = aligned_offset(int_ref.offset, offset_alignment<BType>());
		*out_map_ptr = &dynamic_pools<BType>()[int_ref.pool].host_ptr();
	}

	buffer_binding_args device_memory::index_binding_args(const mesh& object) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const mesh_internal&>(object).index_ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		return { index_pools[ref.pool].buffer(), index_pools[ref.pool].size(), ref.offset, ref.size };
	}

	template<device_memory::buffer_t BType>
	buffer_binding_args device_memory::binding_args(const resource& resource) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const resource_internal&>(resource).ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		return { static_pools<BType>()[ref.pool].buffer(), 0, aligned_offset(ref.offset, offset_alignment<BType>()), 
			ref.size };
	}

	template<device_memory::buffer_t BType>
	buffer_binding_args device_memory::dynamic_binding_args(const resource& resource) noexcept
	{
		const mem_ref_internal& ref
			= reinterpret_cast<const mem_ref_internal&>(reinterpret_cast<const resource_internal&>(resource).ref);
		INTERNAL_ASSERT(ref.valid(), "Invalid memory reference");
		dynamic_buffer_pool& pool = dynamic_pools<BType>()[ref.pool];
		return { pool.buffer(), pool.size(), aligned_offset(ref.offset, offset_alignment<BType>()), ref.size };
	}
}
