#include <pch.hpp>

#include <spiral/render/memory.hpp>
#include <spiral/render/device.hpp>

//NOTE: with the way the system is implemented, having 8MB pools assumes vertexes will have no more than 128 bytes of data

const VkDeviceSize buffer_pool_size = MEGABYTES(8);

const VkDeviceSize texture_pool_size = MEGABYTES(128);

const uint32_t initial_pool_slot_size = 16;

const VkBufferUsageFlags vertex_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
const VkBufferUsageFlags index_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
const VkBufferUsageFlags storage_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
const VkBufferUsageFlags dynamic_buffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
const VkBufferUsageFlags upload_buffer = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
const VkBufferUsageFlags download_buffer = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

const VkMemoryPropertyFlags main_heap = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
const VkMemoryPropertyFlags host_visible_heap = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
const VkMemoryPropertyFlags upload_heap = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags download_heap = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;


namespace Spiral
{
	VkPhysicalDevice* pHandle;
	VkDevice* lHandle;
	VkQueue* gQueue;
	MemoryNexus global;
	VkDeviceSize memGranularity;


	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(*pHandle, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		//do stuff
		DEBUG_BREAK;
		return 0;
	}

	void createPool(VkDeviceMemory& memory, VkBuffer& buffer, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer_pool_size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(*lHandle, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(*lHandle, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(*lHandle, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}
		vkBindBufferMemory(*lHandle, buffer, memory, 0);
	}

	void Memory::init(VkPhysicalDevice* physical, VkDevice* logical, VkQueue* queue, VkCommandPool* pool)
	{
		pHandle = physical;
		lHandle = logical;
		gQueue = queue;
		init(global, pool);
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(*pHandle, &properties);
		memGranularity = properties.limits.nonCoherentAtomSize;
	}

	void Memory::terminate()
	{
	}

	void Memory::init(MemoryNexus& nexus, VkCommandPool* pool)
	{
		nexus.nPools = 1;
		nexus.pools = (memory_pool*)malloc(sizeof(memory_pool));

		nexus.pools[0].n_slots = 0;
		nexus.pools[0].capacity = initial_pool_slot_size;
		nexus.pools[0].slots = (memory_slot*)malloc(sizeof(memory_slot) * initial_pool_slot_size);

		nexus.transientOffset = 0;

		//nexus.pools[0].largestFreeRange.in_use = true;
		//nexus.pools[0].largestFreeRange.offset = 0;
		//nexus.pools[0].largestFreeRange.size = buffer_pool_size;

		createPool(nexus.pools[0].memory, nexus.pools[0].buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		createPool(nexus.transientDeviceMemory, nexus.transientBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		nexus.transientMemory = malloc(buffer_pool_size);
		vkMapMemory(*lHandle, nexus.transientDeviceMemory, 0, VK_WHOLE_SIZE, 0, &nexus.transientMemory);

		nexus.nRanges = 0;
		nexus.transientCapacity = initial_pool_slot_size;
		nexus.ranges = (TransientRange*)malloc(sizeof(TransientRange) * initial_pool_slot_size);

		nexus.cmdPool = pool;

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = *nexus.cmdPool;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(*lHandle, &allocInfo, &nexus.cmdBuffer) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(*lHandle, &fenceInfo, nullptr, &nexus.fence) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}
	}

	void Memory::terminate(MemoryNexus &nexus)
	{
	}

	//TODO: for uniform buffers, use HOST_VISIBLE_BIT, but not HOST_COHERENT_BIT and use vkFlushMappedMemoryRanges during frame setup, all in the same vkBuffer/DeviceMemory with different offsets
	void Memory::createBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, MemoryNexus& nexus = global)
	{
		//TODO: check usage
		uint32_t i, j;
		for (i = 0; i < nexus.nPools; i++)
		{
			/*if (!nexus.pools[i].largestFreeRange.in_use)
			{
				//update largest range
				nexus.pools[i].largestFreeRange.in_use = true;
				nexus.pools[i].largestFreeRange.offset = 36;
				nexus.pools[i].largestFreeRange.size = 100;
			}
			if (nexus.pools[i].largestFreeRange.size >= size)
			{
				break;
			}*/
		}
		if (i == nexus.nPools)
		{
			//add more pools
		}
		for (j = 0; j < nexus.pools[i].n_slots; j++)
		{
			if (!nexus.pools[i].slots[j].in_use)
			{
				break;
			}
		}
		if (j == nexus.pools[i].capacity)
		{
			//add more
		}
		nexus.pools[i].slots[j].in_use = true;
		//nexus.pools[i].slots[j].offset = nexus.pools[i].largestFreeRange.offset;
		nexus.pools[i].slots[j].size = size;

		nexus.pools[i].n_slots++;

		//nexus.pools[i].largestFreeRange.in_use = false;

		if (buffer_pool_size < nexus.transientOffset + size)
		{
			flush(nexus);
		}

		if (nexus.transientCapacity < nexus.nRanges + 1)
		{
			//allocate space
		}

		nexus.ranges[nexus.nRanges].pool = i;
		nexus.ranges[nexus.nRanges].srcOffset = nexus.transientOffset;
		//nexus.ranges[nexus.nRanges].dstOffset = nexus.pools[i].largestFreeRange.offset;
		nexus.ranges[nexus.nRanges].size = size;
		nexus.nRanges++;
		memcpy(static_cast<char*>(nexus.transientMemory) + nexus.transientOffset, data, size);
		nexus.transientOffset += size;
	}

	void Memory::destroyBuffer(uint32_t pool, uint32_t buffer, MemoryNexus& nexus = global)
	{
	}

	void Memory::flush(MemoryNexus& nexus = global)
	{
		if (nexus.transientOffset)
		{
			uint64_t i;
			TransientRange range;
			vkWaitForFences(*lHandle, 1, &nexus.fence, VK_TRUE, UINT64_MAX);

			VkMappedMemoryRange mem = {};
			mem.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mem.memory = nexus.transientDeviceMemory;
			mem.offset = 0;
			mem.size = ((nexus.transientOffset / memGranularity) + 1) * memGranularity;
			vkFlushMappedMemoryRanges(*lHandle, 1, &mem);
			nexus.transientOffset = 0;

			vkResetCommandBuffer(nexus.cmdBuffer, 0);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(nexus.cmdBuffer, &beginInfo);

			//TODO: optimise maybe, if the buffers are the same you can submit multiple regions at once
			for (i = 0; i < nexus.nRanges; i++)
			{
				range = nexus.ranges[i];

				VkBufferCopy copyRegion = {};
				copyRegion.srcOffset = range.srcOffset;
				copyRegion.dstOffset = range.dstOffset;
				copyRegion.size = range.size;
				LOGF_INTERNAL_DEBUG("src:{0} dst:{1} size:{2}", range.srcOffset, range.dstOffset, range.size);
				vkCmdCopyBuffer(nexus.cmdBuffer, nexus.transientBuffer, nexus.pools[range.pool].buffer, 1, &copyRegion);
			}
			nexus.nRanges = 0;

			vkEndCommandBuffer(nexus.cmdBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &nexus.cmdBuffer;

			vkResetFences(*lHandle, 1, &nexus.fence);
			vkQueueSubmit(*gQueue, 1, &submitInfo, nexus.fence);
		}
	}

	void Memory::waitFlush(MemoryNexus& nexus = global)
	{
		vkWaitForFences(*lHandle, 1, &nexus.fence, VK_TRUE, UINT64_MAX);
	}

	class mem_ref_internal : protected memory_reference
	{
	private:
		mem_ref_internal(std::uint32_t pool, std::size_t offset, std::size_t size) : memory_reference(pool, offset, size) { }

	public:
		static inline memory_reference create(std::uint32_t pool, std::size_t offset, std::size_t size)
		{
			return mem_ref_internal(pool, offset, size);
		}
	};

	inline void init_pool_data(memory_pool& pool)
	{
		std::uint32_t i;
		pool.n_slots = initial_pool_slot_size;
		pool.slots = t_malloc<memory_slot>(initial_pool_slot_size);
		pool.slots[0].in_use = false;
		pool.slots[0].offset = 0;
		pool.slots[0].size = buffer_pool_size;
		pool.largest_free_slot = 0;

		for (i = 1; i < pool.n_slots; i++)
		{
			pool.slots[i].in_use = false;
			pool.slots[i].offset = buffer_pool_size;
			pool.slots[i].size = 0;
		}

		pool.pending_copies = t_malloc<VkBufferCopy>(initial_pool_slot_size * max_frames_in_flight);
		pool.pending_copy_capacity = initial_pool_slot_size;
		for (i = 0; i < max_frames_in_flight; i++)
			pool.pending_copy_counts[i] = 0;
	}

	void device_memory::init(device& owner)
	{
		this->owner = &owner;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(owner.physical_handle, &properties);
		mem_granularity = properties.limits.nonCoherentAtomSize;

		vkGetPhysicalDeviceMemoryProperties(owner.physical_handle, &mem_properties);

		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			LOG_INTERNAL_INFO("Memory type " << i << " properties: "
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED " : "")
				<< (mem_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED " : ""))
				//<< std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(mem_properties.memoryTypes[i].propertyFlags))
		}

		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = owner.transfer_idx;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(owner.handle, &pool_info, nullptr, &cmd_pool) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}
		
		VkCommandBufferAllocateInfo cmd_buffer_info = {};
		cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_info.commandPool = cmd_pool;
		cmd_buffer_info.commandBufferCount = max_frames_in_flight;

		if (vkAllocateCommandBuffers(owner.handle, &cmd_buffer_info, cmd_buffers) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		n_pools = 1;
		pools = t_malloc<memory_pool>(1);
		init_pool_data(pools[0]);

		alloc_buffer(pools[0].memory, pools[0].buffer, buffer_pool_size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		for (std::uint32_t i = 0; i < max_frames_in_flight; i++)
		{
			alloc_buffer(device_in[i].device, device_in[i].buffer, buffer_pool_size, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			//device_in.host = ex_malloc(buffer_pool_size); vkmapmemory allocates the memory
			vkMapMemory(owner.handle, device_in[i].device, 0, VK_WHOLE_SIZE, 0, &device_in[i].host);

			VkSemaphoreCreateInfo semaphore_info = {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (vkCreateSemaphore(owner.handle, &semaphore_info, nullptr, &device_in[i].semaphore) != VK_SUCCESS
				|| vkCreateSemaphore(owner.handle, &semaphore_info, nullptr, &device_out[i].semaphore) != VK_SUCCESS)
			{
				//do stuff
				DEBUG_BREAK;
			}

			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateFence(owner.handle, &fence_info, nullptr, &device_in[i].fence) != VK_SUCCESS
				|| vkCreateFence(owner.handle, &fence_info, nullptr, &device_out[i].fence) != VK_SUCCESS)
			{
				//do stuff
				DEBUG_BREAK;
			}

			device_in[i].offset = 0;
		}
	}

	void device_memory::terminate()
	{
		std::uint32_t i;
		for (i = 0; i < n_pools; i++)
		{
			std::free(pools[i].slots);
			std::free(pools[i].pending_copies);
			vkDestroyBuffer(owner->handle, pools[i].buffer, nullptr);
			vkFreeMemory(owner->handle, pools[i].memory, nullptr);
		}
		for (i = 0; i < max_frames_in_flight; i++)
		{
			vkUnmapMemory(owner->handle, device_in[i].device);
			vkDestroyBuffer(owner->handle, device_in[i].buffer, nullptr);
			vkFreeMemory(owner->handle, device_in[i].device, nullptr);
			vkDestroySemaphore(owner->handle, device_in[i].semaphore, nullptr);
			vkDestroyFence(owner->handle, device_in[i].fence, nullptr);

			//vkDestroyBuffer(owner->handle, device_out[i].buffer, nullptr);
			vkFreeMemory(owner->handle, device_out[i].device, nullptr);
			vkDestroySemaphore(owner->handle, device_out[i].semaphore, nullptr);
			vkDestroyFence(owner->handle, device_out[i].fence, nullptr);
		}
		std::free(pools);
		vkDestroyCommandPool(owner->handle, cmd_pool, nullptr);
	}

	void device_memory::flush_in(const std::uint16_t current_frame)
	{
		transfer_memory& memory = device_in[current_frame];
		if (memory.offset)
		{
			//TODO: if not host coherent...

			vkResetCommandBuffer(cmd_buffers[current_frame], 0);

			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(cmd_buffers[current_frame], &begin_info);

			for (std::uint32_t pool_idx = 0; pool_idx < n_pools; pool_idx++)
			{
				memory_pool& pool = pools[pool_idx];
				vkCmdCopyBuffer(cmd_buffers[current_frame], memory.buffer, pool.buffer, pool.pending_copy_counts[current_frame], 
					&pool.pending_copies[pool.pending_copy_capacity * current_frame]);
				pool.pending_copy_counts[current_frame] = 0;
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
	}

	void device_memory::synchronize()
	{
		vkWaitForFences(owner->handle, 1, &device_in[owner->current_frame].fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
	}

	inline void search_pools(memory_pool* pools, const std::uint32_t n_pools, const VkDeviceSize size,
		std::uint32_t& out_pool_idx, std::uint32_t& out_slot_idx)
	{
		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		for (std::uint32_t pool_idx = 0; pool_idx < n_pools; pool_idx++)
		{
			memory_pool& pool = pools[pool_idx];
			if (pool.slots[pool.largest_free_slot].in_use)
			{
				VkDeviceSize largest_fit = 0;
				std::uint32_t largest_idx = 0;
				VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
				std::uint32_t smallest_idx = std::numeric_limits<std::uint32_t>::max();
				for (std::uint32_t slot_idx = 0; slot_idx < pool.n_slots; slot_idx++)
				{
					if (!pool.slots[slot_idx].in_use)
					{
						if (largest_fit < pool.slots[slot_idx].size)
						{
							largest_fit = pool.slots[slot_idx].size;
							largest_idx = slot_idx;
						}
						if (size <= pool.slots[slot_idx].size && smallest_fit > pool.slots[slot_idx].size)
						{
							smallest_fit = pool.slots[slot_idx].size;
							smallest_idx = slot_idx;
						}
					}
				}
				if (size <= smallest_fit)
				{
					selected_pool_idx = pool_idx;
					selected_slot_idx = smallest_idx;
					if (!(largest_idx == smallest_idx))
						pool.largest_free_slot = largest_idx;
					break;
				}
				else
				{
					pool.largest_free_slot = largest_idx;
					continue;
				}
			}
			else
			{
				if (pool.slots[pool.largest_free_slot].size < size)
					continue;

				VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
				std::uint32_t smallest_idx = std::numeric_limits<std::uint32_t>::max();
				for (std::uint32_t slot_idx = 0; slot_idx < pool.n_slots; slot_idx++)
				{
					if (!pool.slots[slot_idx].in_use && size <= pool.slots[slot_idx].size && smallest_fit > pool.slots[slot_idx].size)
					{
						smallest_fit = pool.slots[slot_idx].size;
						smallest_idx = slot_idx;
					}
				}
				selected_pool_idx = pool_idx;
				selected_slot_idx = smallest_idx;
				break;
			}
		}
		out_pool_idx = selected_pool_idx;
		out_slot_idx = selected_slot_idx;
	}

	inline void fill_slot(memory_pool& pool, const std::uint32_t slot_idx, const VkDeviceSize size)
	{
		if (size < pool.slots[slot_idx].size)
		{
			const std::uint32_t old_size = pool.n_slots;
			bool selected_last;
			if (selected_last = (slot_idx == (pool.n_slots - 1)) ||
				(pool.slots[slot_idx + 1].in_use && pool.slots[pool.n_slots - 1].in_use))
			{
				pool.n_slots += initial_pool_slot_size; //check if this is the best way to increment
				pool.slots = t_realloc<memory_slot>(pool.slots, pool.n_slots);
				for (std::uint32_t i = old_size; i < pool.n_slots; i++)
				{
					pool.slots[i].in_use = false;
					pool.slots[i].offset = pool.slots[old_size - 1].offset + pool.slots[old_size - 1].size;
					pool.slots[i].size = 0;
				}
			}
			if (!selected_last)
			{
				std::uint32_t last_in_use = 0;
				for (std::uint32_t i = old_size; i != std::numeric_limits<std::uint32_t>::max(); i--)
				{
					if (pool.slots[i].in_use)
					{
						last_in_use = i;
						break;
					}
				}
				std::memmove(&pool.slots[slot_idx + 2], &pool.slots[slot_idx + 1],
					sizeof(memory_slot) * (last_in_use - slot_idx));
			}
		}

		pool.slots[slot_idx + 1].offset -= pool.slots[slot_idx].size - size;
		pool.slots[slot_idx + 1].size += pool.slots[slot_idx].size - size;
		pool.slots[slot_idx].in_use = true;
		pool.slots[slot_idx].size = size;
	}

	inline void submit_upload(memory_pool& pool, std::uint32_t slot_idx, transfer_memory& memory, 
		const std::uint16_t current_frame, const void* data, const VkDeviceSize size)
	{
		if (memory.offset + size > buffer_pool_size)
		{
			//flush_in(current_frame); //TODO: deal with not having enough transfer memory better
			//vkWaitForFences(owner->handle, 1, &memory.fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
		}
		else if (pool.pending_copy_capacity <= pool.pending_copy_counts[current_frame])
		{
			const std::uint32_t old_cap = pool.pending_copy_capacity;
			pool.pending_copy_capacity += initial_pool_slot_size;
			pool.pending_copies = t_realloc<VkBufferCopy>(pool.pending_copies,
				static_cast<std::uint64_t>(pool.pending_copy_capacity) * max_frames_in_flight);
			for (std::uint32_t i = max_frames_in_flight - 1; i > 0; i--)
			{
				std::memmove(&pool.pending_copies[pool.pending_copy_capacity * i],
					&pool.pending_copies[old_cap * i], sizeof(VkBufferCopy) * pool.pending_copy_counts[i]);
			}
		}

		VkBufferCopy& info = pool.pending_copies[pool.pending_copy_capacity * current_frame + pool.pending_copy_counts[current_frame]];
		info.srcOffset = memory.offset;
		info.dstOffset = pool.slots[slot_idx].offset;
		info.size = size;

		std::memcpy(reinterpret_cast<char*>(memory.host) + memory.offset, data, size);

		memory.offset += size;
		pool.pending_copy_counts[current_frame]++;
	}
	//TODO: change refs to pointers
	template<VkBufferUsageFlags BFlags, VkMemoryPropertyFlags MFlags>
	inline void device_memory::alloc_slot(memory_pool* pools, std::uint32_t& n_pools,
		std::uint32_t& pool_idx, std::uint32_t& slot_idx, const VkDeviceSize size)
	{
		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		search_pools(pools, n_pools, size, selected_pool_idx, selected_slot_idx);
		if (selected_pool_idx == std::numeric_limits<std::uint32_t>::max())
		{
			selected_pool_idx = n_pools;
			selected_slot_idx = 0;
			n_pools++;
			pools = t_realloc<memory_pool>(pools, n_pools);
			init_pool_data(pools[selected_pool_idx]);
			alloc_buffer(pools[selected_pool_idx].memory, pools[selected_pool_idx].buffer, buffer_pool_size, BFlags, MFlags);
		}

		fill_slot(pools[selected_pool_idx], selected_slot_idx, size);
		pool_idx = selected_pool_idx;
		slot_idx = selected_slot_idx;
	}

	memory_reference device_memory::alloc_object(const VkDeviceSize size)
	{
		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		alloc_slot<vertex_buffer, main_heap>(pools, n_pools, selected_pool_idx, selected_slot_idx, size);
		return mem_ref_internal::create(selected_pool_idx, pools[selected_pool_idx].slots[selected_slot_idx].offset, size);
	}

	memory_reference device_memory::ialloc_object(const void* data, const VkDeviceSize size)
	{
		std::uint32_t selected_pool_idx = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t selected_slot_idx = std::numeric_limits<std::uint32_t>::max();
		alloc_slot<vertex_buffer, main_heap>(pools, n_pools, selected_pool_idx, selected_slot_idx, size);
		memory_pool& selected_pool = pools[selected_pool_idx];
		Spiral::submit_upload(selected_pool, selected_slot_idx, device_in[owner->current_frame], owner->current_frame, data, size);
		return mem_ref_internal::create(selected_pool_idx, selected_pool.slots[selected_slot_idx].offset, size);
	}

	void device_memory::submit_upload(const memory_reference& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset)
	{

	}

	uint32_t device_memory::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		//Exact type search
		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && mem_properties.memoryTypes[i].propertyFlags == properties)
			{
				return i;
			}
		}
		LOG_INTERNAL_WARN("Failed to find exact memory type.")
		//Relaxed search
		for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
		{
			if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		//do stuff
		//DEBUG_BREAK;
		LOG_INTERNAL_ERROR("Failed to find suitable memory type. Type bit mask: " 
			<< std::bitset<sizeof(type_filter) * 8>(type_filter) << " Property flags: "
			<< std::bitset<sizeof(properties) * 8>(properties))
		return std::numeric_limits<uint32_t>::max();
	}

	void device_memory::alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(owner->handle, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(owner->handle, buffer, &memory_requirements);

		VkMemoryAllocateInfo memory_info = {};
		memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_info.allocationSize = memory_requirements.size;
		memory_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

		if (vkAllocateMemory(owner->handle, &memory_info, nullptr, &memory) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}
		vkBindBufferMemory(owner->handle, buffer, memory, 0);
	}
}
