#pragma once

#include "render_core.hpp"
#include "memory_reference.hpp"

namespace Spiral
{
	class device;

	struct memory_slot
	{
		bool in_use;
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	struct TransientRange
	{
		uint32_t pool;
		uint32_t srcOffset;
		uint32_t dstOffset;
		VkDeviceSize size;
	};

	struct transfer_memory
	{
		VkDeviceMemory device;
		VkBuffer buffer;
		void* host;
		VkSemaphore semaphore;
		VkFence fence;
		bool ready;
		VkDeviceSize offset;
	};

	struct memory_pool
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
		memory_slot* slots;
		std::uint32_t n_slots;
		std::uint32_t capacity;
		std::uint32_t largest_free_slot;
		VkBufferCopy* pending_copies;
		std::uint32_t pending_copy_counts[max_frames_in_flight];
		std::uint32_t pending_copy_capacity;
	};

	struct MemoryNexus
	{
		memory_pool* pools;
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

	class device_memory
	{
	public:
		~device_memory();

		void init(device& owner);
		void terminate();

		//void update_largest_slots();
		//void tick(); could maybe replace the above function and performa all updates and cleanup

		void flush_in(const std::uint16_t current_frame);
		//void flush_out();

		void synchronize();

		memory_reference alloc_object(const VkDeviceSize size);
		memory_reference ialloc_object(const void* data, const VkDeviceSize size);
		//memory_reference alloc_dynamic_object();
		//memory_reference alloc_indexes();
		//memory_reference alloc_storage();
		//memory_reference alloc_texture();

		void submit_upload(const memory_reference& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);
		//void upload(const memory_reference& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);

	private:
		std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);

		void alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		template<VkBufferUsageFlags BFlags, VkMemoryPropertyFlags MFlags>
		void alloc_slot(memory_pool* pools, const std::uint32_t& n_pools,
			std::uint32_t& pool_idx, std::uint32_t& slot_idx, const VkDeviceSize size);

		device* owner = nullptr;

		VkDeviceSize mem_granularity; //from physical device properties
		VkPhysicalDeviceMemoryProperties mem_properties;

		VkCommandPool cmd_pool;
		VkCommandBuffer cmd_buffers[max_frames_in_flight];

		transfer_memory device_in[max_frames_in_flight];
		transfer_memory device_out[max_frames_in_flight];

	public:
		std::uint32_t n_pools = 1;
		memory_pool* pools;
	};
}
