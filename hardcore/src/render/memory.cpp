#include <pch.hpp>

#include <render/memory.hpp>
#include <render/device.hpp>

#include <debug/log_internal.hpp>

const VkDeviceSize staging_buffer_size = MEGABYTES(8);

const VkDeviceSize texture_pool_size = MEGABYTES(128);

const VkBufferUsageFlags upload_buffer = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
const VkBufferUsageFlags download_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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
	template<>
	struct buffer<device_memory::buffer_t::UNIVERSAL>
	{
		static constexpr const char* debug_name = "UNIVERSAL";
		static const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		static const VkDeviceSize size = MEGABYTES(8);
		static const VkBufferUsageFlags dynamic_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		static const VkDeviceSize dynamic_size = MEGABYTES(8);
	};

	template<bool Dynamic>
	struct get_buffer_pool {};
	template<> struct get_buffer_pool<false> { using type = buffer_pool; };
	template<> struct get_buffer_pool<true> { using type = dynamic_buffer_pool; };

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

	inline VkDeviceSize increase_to_fit(VkDeviceSize base, VkDeviceSize target)
	{
		//mathematical equivalent to a loop doubling base until it can fit target
		const double exp = std::ceil(std::log2(static_cast<double>(target) / base));
		return base * std::exp2(exp);
	}

	template<typename Pool>
	inline bool search_pools(const std::vector<Pool>& pools, const VkDeviceSize size, VkDeviceSize alignment,
		std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed)
	{
		static_assert(std::is_base_of<memory_pool, Pool>::value, "Invalid pool type");

		std::uint32_t selected_pool_idx = 0;
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		for (const memory_pool& pool : pools)
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

	void device_memory::init(VkPhysicalDevice physical_device, VkDevice device, std::uint32_t transfer_queue_idx,
		const VkPhysicalDeviceLimits* limits, const std::uint8_t* current_frame)
	{
		update_refs(device, limits, current_frame);

		heap_manager = device_heap_manager(physical_device);

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

		texture_pools.resize(heap_manager.mem_properties().memoryTypeCount);

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			heap_manager.alloc_buffer(device, device_in[i].device, device_in[i].buffer, staging_buffer_size, 
				upload_buffer,
				device_heap_manager::heap::UPLOAD);

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
		for (buffer_pool& pool : vertex_pools) pool.free(device, heap_manager);
		vertex_pools.clear();
		for (buffer_pool& pool : index_pools) pool.free(device, heap_manager);
		index_pools.clear();
		for (buffer_pool& pool : uniform_pools) pool.free(device, heap_manager);
		uniform_pools.clear();
		for (buffer_pool& pool : storage_pools) pool.free(device, heap_manager);
		storage_pools.clear();
		for (buffer_pool& pool : d_vertex_pools) pool.free(device, heap_manager);
		d_vertex_pools.clear();
		for (buffer_pool& pool : d_index_pools) pool.free(device, heap_manager);
		d_index_pools.clear();
		for (buffer_pool& pool : d_uniform_pools) pool.free(device, heap_manager);
		d_uniform_pools.clear();
		for (buffer_pool& pool : d_storage_pools) pool.free(device, heap_manager);
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
			for (const auto& [path, copies] : vp_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, path.first),
					vertex_pools[path.second].buffer(), static_cast<std::uint32_t>(copies.size()), copies.data());
			}
			for (const auto& [path, copies] : ip_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, path.first),
					index_pools[path.second].buffer(), static_cast<std::uint32_t>(copies.size()), copies.data());
			}
			for (const auto& [path, copies] : up_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, path.first),
					uniform_pools[path.second].buffer(), static_cast<std::uint32_t>(copies.size()), copies.data());
			}
			for (const auto& [path, copies] : sp_pending_copies[current_frame])
			{
				vkCmdCopyBuffer(cmd_buffers[current_frame], get_staging_buffer(memory, path.first),
					storage_pools[path.second].buffer(), static_cast<std::uint32_t>(copies.size()), copies.data());
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

	memory_ref device_memory::alloc_vertices(const VkDeviceSize size)
	{ return alloc_buffer<VERTEX, false>(size); }
	memory_ref device_memory::alloc_vertices(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<VERTEX, false>(data, size); }
	memory_ref device_memory::alloc_indexes(const VkDeviceSize size)
	{ return alloc_buffer<INDEX, false>(size); }
	memory_ref device_memory::alloc_indexes(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<INDEX, false>(data, size); }
	memory_ref device_memory::alloc_uniform(const VkDeviceSize size)
	{ return alloc_buffer<UNIFORM, false>(size); }
	memory_ref device_memory::alloc_uniform(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<UNIFORM, false>(data, size); }
	memory_ref device_memory::alloc_storage(const VkDeviceSize size)
	{ return alloc_buffer<STORAGE, false>(size); }
	memory_ref device_memory::alloc_storage(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<STORAGE, false>(data, size); }

	memory_ref device_memory::alloc_dynamic_vertices(const VkDeviceSize size)
	{ return alloc_buffer<VERTEX, true>(size); }
	memory_ref device_memory::alloc_dynamic_vertices(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<VERTEX, true>(data, size); }
	memory_ref device_memory::alloc_dynamic_indexes(const VkDeviceSize size)
	{ return alloc_buffer<INDEX, true>(size); }
	memory_ref device_memory::alloc_dynamic_indexes(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<INDEX, true>(data, size); }
	memory_ref device_memory::alloc_dynamic_uniform(const VkDeviceSize size)
	{ return alloc_buffer<UNIFORM, true>(size); }
	memory_ref device_memory::alloc_dynamic_uniform(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<UNIFORM, true>(data, size); }
	memory_ref device_memory::alloc_dynamic_storage(const VkDeviceSize size)
	{ return alloc_buffer<STORAGE, true>(size); }
	memory_ref device_memory::alloc_dynamic_storage(const void* data, const VkDeviceSize size)
	{ return alloc_buffer<STORAGE, true>(data, size); }

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

	template<device_memory::buffer_t BType, bool Dynamic>
	inline memory_ref device_memory::alloc_buffer(VkDeviceSize size)
	{
		using pool_t = typename get_buffer_pool<Dynamic>::type;
		const VkDeviceSize alignment = offset_alignment<BType>();
		std::vector<pool_t>* pools;
		VkDeviceSize pool_size;
		if constexpr (!Dynamic)
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
				pool_size = increase_to_fit(pool_size, size);

			if constexpr (!Dynamic || BType == buffer_t::UNIVERSAL)
			{
				pools->push_back(buffer_pool(radd.device, heap_manager, pool_size, buffer<BType>::usage, 
					device_heap_manager::heap::MAIN));
			}
			else
			{
				pools->push_back(dynamic_buffer_pool(radd.device, heap_manager, pool_size, 
					buffer<BType>::dynamic_usage, device_heap_manager::heap::DYNAMIC));
				pool_t& pool = (*pools)[selected_pool_idx];
				pool.map(radd.device, *radd.current_frame);
			}
		}

		(*pools)[selected_pool_idx].fill_slot(selected_slot_idx, size_needed);

		LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} (offset {2}->{3}) of pool {4} {5}",
			size, selected_slot_idx,
			(*pools)[selected_pool_idx][selected_slot_idx].offset,
			aligned_offset((*pools)[selected_pool_idx][selected_slot_idx].offset, alignment),
			buffer<BType>::debug_name, selected_pool_idx);

		return mem_ref_internal(BType, selected_pool_idx, (*pools)[selected_pool_idx][selected_slot_idx].offset, size);
	}

	template<device_memory::buffer_t BType, bool Dynamic>
	inline memory_ref device_memory::alloc_buffer(const void* data, VkDeviceSize size)
	{
		memory_ref ref = alloc_buffer<BType, Dynamic>(size);
		const mem_ref_internal& int_ref = reinterpret_cast<const mem_ref_internal&>(ref);

		if constexpr (!Dynamic || BType == buffer_t::UNIVERSAL)
			submit_upload<BType>(int_ref.pool, int_ref.offset, data, size);
		else
			this->memcpy<BType>(ref, data, size, 0);

		return ref;
	}

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

	template<bool Dynamic>
	inline memory_ref device_memory::alloc_texture(VkDeviceSize size)
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

		VkMemoryRequirements requirements;
		texture tex = create_texture(radd.device, image_info, &requirements);

		using pool_t = typename get_buffer_pool<Dynamic>::type;
		const VkDeviceSize alignment = offset_alignment<BType>();
		std::vector<pool_t>* pools;
		VkDeviceSize pool_size;
		if constexpr (!Dynamic)
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
				pool_size = increase_to_fit(pool_size, size);

			if constexpr (!Dynamic || BType == buffer_t::UNIVERSAL)
			{
				constexpr bool per_frame_allocation = BType == buffer_t::UNIVERSAL;
				pools->push_back(buffer_pool(radd.device, heap_manager, pool_size,
					buffer<BType>::usage, main_heap, per_frame_allocation));
			}
			else
			{
				pools->push_back(dynamic_buffer_pool(radd.device, heap_manager, pool_size,
					buffer<BType>::dynamic_usage, host_visible_heap));
				pool_t& pool = (*pools)[selected_pool_idx];
				pool.map(radd.device, *radd.current_frame);
			}
		}

		(*pools)[selected_pool_idx].fill_slot(selected_slot_idx, size_needed);

		LOGF_INTERNAL_INFO("Allocated {0} bytes in slot {1} (offset {2}->{3}) of pool {4} {5}",
			size, selected_slot_idx,
			(*pools)[selected_pool_idx][selected_slot_idx].offset,
			aligned_offset((*pools)[selected_pool_idx][selected_slot_idx].offset, alignment),
			buffer<BType>::debug_name, selected_pool_idx);

		return mem_ref_internal(BType, selected_pool_idx, (*pools)[selected_pool_idx][selected_slot_idx].offset, size);
	}
}
