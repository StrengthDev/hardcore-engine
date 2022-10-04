#pragma once

#include "render_core.hpp"
#include "memory_reference.hpp"
#include "resource.hpp"
#include "instance.hpp"

namespace Spiral
{
	class device;

	struct memory_slot
	{
		bool in_use;
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	struct transfer_memory
	{
		VkDeviceMemory device;
		VkBuffer buffer;
		void* host = nullptr;
		VkSemaphore semaphore;
		VkFence fence;
		bool ready;
		VkDeviceSize offset;
	};

	struct memory_pool
	{
		VkDeviceMemory memory;
		VkBuffer buffer;
		memory_slot* slots = nullptr;
		std::uint32_t n_slots;
		std::uint32_t capacity; //TODO: remove
		std::uint32_t largest_free_slot;
		VkBufferCopy* pending_copies = nullptr;
		std::uint32_t pending_copy_counts[max_frames_in_flight];
		std::uint32_t pending_copy_capacity;
	};

	struct buffer_binding_args
	{
		VkBuffer buffer;
		VkDeviceSize size;
		VkDeviceSize offset;
	};

	class device_memory
	{
	public:
		void init(device& owner);
		void terminate();

		//void update_largest_slots();
		//void tick(); could maybe replace the above function and performa all updates and cleanup

		void flush_in(const std::uint8_t current_frame);
		//void flush_out();

		void map_ranges(const std::uint8_t current_frame);
		void unmap_ranges(const std::uint8_t current_frame);

		void synchronize(const std::uint8_t current_frame);

		memory_reference alloc_object(const VkDeviceSize size);
		memory_reference ialloc_object(const void* data, const VkDeviceSize size);
		memory_reference alloc_dynamic_object(const VkDeviceSize size);
		//memory_reference alloc_indexes();
		//memory_reference alloc_storage();
		//memory_reference alloc_texture();

		void submit_upload(const memory_reference& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);
		//void upload(const memory_reference& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);

		buffer_binding_args get_binding_args(const object_resource& object);
		buffer_binding_args get_binding_args(const instance& instance);

		inline VkSemaphore get_device_in_semaphore(const std::uint8_t current_frame) { return device_in[current_frame].semaphore; }

	private:
		std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);

		void alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, 
			VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		template<VkBufferUsageFlags BFlags, VkMemoryPropertyFlags MFlags>
		void alloc_slot(memory_pool* pools, std::uint32_t* out_n_pools,
			std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize size); //this function is only used by the class, so it's not a problem that it isn't defined in the header

		device* owner = nullptr;

		VkDeviceSize mem_granularity; //from physical device properties
		VkPhysicalDeviceMemoryProperties mem_properties;

		VkCommandPool cmd_pool;
		VkCommandBuffer cmd_buffers[max_frames_in_flight];

		transfer_memory device_in[max_frames_in_flight];
		transfer_memory device_out[max_frames_in_flight];

		std::uint32_t n_pools;
		memory_pool* pools = nullptr;
	};
}
