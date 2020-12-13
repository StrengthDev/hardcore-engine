#pragma once

#include "RenderCore.hpp"

//NOTE: with the way the system is implemented, having 8MB pools assumes vertexes will have no more than 128 bytes of data
//TODO: use push constants for model matrix and other object related variables, uniform buffer for view, world matrices and such, regular buffers for the rest

namespace Spiral
{
	struct Range
	{
		bool valid;
		uint32_t offset;
		VkDeviceSize size;
	};

	struct TransientRange
	{
		uint32_t pool;
		uint32_t srcOffset;
		uint32_t dstOffset;
		VkDeviceSize size;
	};

	struct Pool
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
		Range* ranges;
		uint32_t nRanges;
		uint32_t capacity;
		Range largestFreeRange;
	};

	struct MemoryNexus
	{
		Pool* pools;
		uint32_t nPools;
		uint32_t capacity;
		VkDeviceMemory transientDeviceMemory;
		VkBuffer transientBuffer;
		void* transientMemory;
		VkDeviceSize transientOffset;
		TransientRange* ranges;
		uint32_t nRanges;
		uint32_t transientCapacity;
		VkCommandPool* cmdPool;
		VkCommandBuffer cmdBuffer;
		VkFence fence;
	};

	namespace Memory
	{
		void init(VkPhysicalDevice *physical, VkDevice *logical, VkQueue *queue, VkCommandPool *pool);
		void terminate();

		void init(MemoryNexus &nexus, VkCommandPool* pool);
		void terminate(MemoryNexus &nexus);

		void createBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags usage, MemoryNexus &nexus);
		void destroyBuffer(uint32_t pool, uint32_t buffer, MemoryNexus &nexus);

		void flush(MemoryNexus &nexus);
		void waitFlush(MemoryNexus &nexus);

		//NOTE: May want to add a function for pre allocation
	};
}