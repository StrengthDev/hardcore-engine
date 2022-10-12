#pragma once

#include "render_core.hpp"
#include "memory_reference.hpp"
#include "resource.hpp"
#include "instance.hpp"

namespace ENGINE_NAMESPACE
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

	class memory_pool
	{
	public:
		memory_pool() = default;
		memory_pool(VkDevice& handle);
		~memory_pool();

		void free(VkDevice& handle);

		bool search(const VkDeviceSize size, std::uint32_t* out_slot_idx);
		void fill_slot(const std::uint32_t slot_idx, const VkDeviceSize size);

		memory_pool(const memory_pool&) = delete;
		memory_pool& operator=(const memory_pool&) = delete;

		memory_pool(memory_pool&& other) noexcept
			: memory(std::exchange(other.memory, VK_NULL_HANDLE)),
			buffer(std::exchange(other.buffer, VK_NULL_HANDLE)),
			slots(std::exchange(other.slots, nullptr)),
			n_slots(std::exchange(other.n_slots, 0)),
			capacity(std::exchange(other.capacity, 0)),
			largest_free_slot(std::exchange(other.largest_free_slot, 0))
		{ }

		memory_pool& operator=(memory_pool&& other) noexcept
		{
			this->~memory_pool();
			memory = std::exchange(other.memory, VK_NULL_HANDLE);
			buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
			slots = std::exchange(other.slots, nullptr);
			n_slots = std::exchange(other.n_slots, 0);
			capacity = std::exchange(other.capacity, 0);
			largest_free_slot = std::exchange(other.largest_free_slot, 0);
		}

		inline VkDeviceMemory& get_memory() noexcept { return memory; }
		inline VkBuffer& get_buffer() noexcept { return buffer; }
		inline const memory_slot& get_slot(std::uint32_t idx) const { return slots[idx]; }

	private:
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		memory_slot* slots = nullptr;
		std::uint32_t n_slots;
		std::uint32_t capacity; //TODO: remove
		std::uint32_t largest_free_slot;
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
		buffer_binding_args get_binding_args(const device_data& instance);

		inline VkSemaphore get_device_in_semaphore(const std::uint8_t current_frame) { return device_in[current_frame].semaphore; }

	private:

		enum class buffer_t : std::uint8_t
		{
			NONE = 0,
			VERTEX,
		};

		typedef std::pair<std::uint32_t, std::uint32_t> copy_path_t;

		struct hash
		{
			inline std::size_t operator()(const copy_path_t& key) const noexcept
			{
				return static_cast<std::size_t>((static_cast<std::uint64_t>(key.first) << 32) | key.second);
			}
		};

		typedef std::unordered_map<copy_path_t, std::vector<VkBufferCopy>, hash> pending_copies_t;

		std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);

		void alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, 
			VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		template<VkBufferUsageFlags BFlags, VkMemoryPropertyFlags MFlags>
		void alloc_slot(std::vector<memory_pool>& pools,
			std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize size); //this function is only used by the class, so it's not a problem that it isn't defined in the header

		template<buffer_t BType>
		inline std::vector<memory_pool>& get_pools() { static_assert(false, "Unimplemented buffer type"); }
		template<>
		inline std::vector<memory_pool>& get_pools<buffer_t::VERTEX>() noexcept { return vertex_pools; }

		template<buffer_t BType>
		inline pending_copies_t& get_pending_copies(std::uint16_t current_frame) 
		{
			static_assert(false, "Unimplemented buffer type");
		}
		template<>
		inline pending_copies_t& get_pending_copies<buffer_t::VERTEX>(std::uint16_t current_frame) noexcept
		{
			return vp_pending_copies[current_frame];
		}

		template<buffer_t BType>
		void submit_upload(std::uint32_t pool_idx, std::uint32_t slot_idx, const void* data, const VkDeviceSize size);

		device* owner = nullptr;

		VkDeviceSize mem_granularity; //from physical device properties
		VkPhysicalDeviceMemoryProperties mem_properties;

		VkCommandPool cmd_pool;
		VkCommandBuffer cmd_buffers[max_frames_in_flight];

		transfer_memory device_in[max_frames_in_flight]; //TODO: move all transfer memories to a single buffer and change offset depending on frame
		transfer_memory device_out[max_frames_in_flight];

		std::vector<memory_pool> vertex_pools;

		pending_copies_t vp_pending_copies[max_frames_in_flight];
	};
}
