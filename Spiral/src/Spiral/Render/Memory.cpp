#include "pch.hpp"

#include "Memory.hpp"

//8 Megabytes
#define BUFFER_POOL_SIZE BIT(23)

//128 Megabytes
#define TEXTURE_POOL_SIZE BIT(27)

#define INITIAL_POOL_SLOT_SIZE 4


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
		bufferInfo.size = BUFFER_POOL_SIZE;
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
		nexus.pools = (Pool*)malloc(sizeof(Pool));

		nexus.pools[0].nRanges = 0;
		nexus.pools[0].capacity = INITIAL_POOL_SLOT_SIZE;
		nexus.pools[0].ranges = (Range*)malloc(sizeof(Range) * INITIAL_POOL_SLOT_SIZE);

		nexus.transientOffset = 0;

		nexus.pools[0].largestFreeRange.valid = true;
		nexus.pools[0].largestFreeRange.offset = 0;
		nexus.pools[0].largestFreeRange.size = BUFFER_POOL_SIZE;

		createPool(nexus.pools[0].memory, nexus.pools[0].buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		createPool(nexus.transientDeviceMemory, nexus.transientBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		nexus.transientMemory = malloc(BUFFER_POOL_SIZE);
		vkMapMemory(*lHandle, nexus.transientDeviceMemory, 0, BUFFER_POOL_SIZE, 0, &nexus.transientMemory);

		nexus.nRanges = 0;
		nexus.transientCapacity = INITIAL_POOL_SLOT_SIZE;
		nexus.ranges = (TransientRange*)malloc(sizeof(TransientRange) * INITIAL_POOL_SLOT_SIZE);

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
			if (!nexus.pools[i].largestFreeRange.valid)
			{
				//update largest range
				nexus.pools[i].largestFreeRange.valid = true;
				nexus.pools[i].largestFreeRange.offset = 36;
				nexus.pools[i].largestFreeRange.size = 100;
			}
			if (nexus.pools[i].largestFreeRange.size >= size)
			{
				break;
			}
		}
		if (i == nexus.nPools)
		{
			//add more pools
		}
		for (j = 0; j < nexus.pools[i].nRanges; j++)
		{
			if (!nexus.pools[i].ranges[j].valid)
			{
				break;
			}
		}
		if (j == nexus.pools[i].capacity)
		{
			//add more
		}
		nexus.pools[i].ranges[j].valid = true;
		nexus.pools[i].ranges[j].offset = nexus.pools[i].largestFreeRange.offset;
		nexus.pools[i].ranges[j].size = size;

		nexus.pools[i].nRanges++;

		nexus.pools[i].largestFreeRange.valid = false;

		if (BUFFER_POOL_SIZE < nexus.transientOffset + size)
		{
			flush(nexus);
		}

		if (nexus.transientCapacity < nexus.nRanges + 1)
		{
			//allocate space
		}

		nexus.ranges[nexus.nRanges].pool = i;
		nexus.ranges[nexus.nRanges].srcOffset = nexus.transientOffset;
		nexus.ranges[nexus.nRanges].dstOffset = nexus.pools[i].largestFreeRange.offset;
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
				copyRegion.srcOffset = range.srcOffset; // Optional
				copyRegion.dstOffset = range.dstOffset; // Optional
				copyRegion.size = range.size;
				SPRL_CORE_DEBUG("src:{0} dst:{1} size:{2}", range.srcOffset, range.dstOffset, range.size);
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
}
