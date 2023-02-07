#pragma once

#include "render_core.hpp"
#include <render/memory_ref.hpp>
#include <render/resource.hpp>

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
		memory_pool(VkDevice& handle, VkDeviceSize size);
		~memory_pool();

		void free(VkDevice& handle);

		bool search(const VkDeviceSize size, std::uint32_t* out_slot_idx);
		void fill_slot(const std::uint32_t slot_idx, const VkDeviceSize size);

		memory_pool(const memory_pool&) = delete;
		memory_pool& operator=(const memory_pool&) = delete;

		memory_pool(memory_pool&& other) noexcept :
			memory(std::exchange(other.memory, VK_NULL_HANDLE)),
			buffer(std::exchange(other.buffer, VK_NULL_HANDLE)),
			size(std::exchange(other.size, 0)),
			slots(std::exchange(other.slots, nullptr)),
			n_slots(std::exchange(other.n_slots, 0)),
			largest_free_slot(std::exchange(other.largest_free_slot, 0))
		{ }

		inline memory_pool& operator=(memory_pool&& other) noexcept
		{
			this->~memory_pool();
			memory = std::exchange(other.memory, VK_NULL_HANDLE);
			buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
			size = std::exchange(other.size, 0);
			slots = std::exchange(other.slots, nullptr);
			n_slots = std::exchange(other.n_slots, 0);
			largest_free_slot = std::exchange(other.largest_free_slot, 0);
		}

		inline VkDeviceMemory& get_memory() noexcept { return memory; }
		inline VkBuffer& get_buffer() noexcept { return buffer; }
		inline VkDeviceSize get_size() const noexcept { return size; }
		inline const memory_slot& get_slot(std::uint32_t idx) const { return slots[idx]; }

	private:
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		memory_slot* slots = nullptr;
		std::uint32_t n_slots = 0;
		std::uint32_t largest_free_slot = 0;
	};

	class dynamic_memory_pool : public memory_pool
	{
	public:
		dynamic_memory_pool() = default;
		dynamic_memory_pool(VkDevice& handle, VkDeviceSize size) : memory_pool(handle, size), host_ptr(nullptr) {}
		~dynamic_memory_pool();

		dynamic_memory_pool(dynamic_memory_pool&& other) noexcept :
			memory_pool(std::move(other)),
			host_ptr(std::exchange(other.host_ptr, nullptr))
		{ }

		inline dynamic_memory_pool& operator=(dynamic_memory_pool&& other) noexcept
		{
			this->~dynamic_memory_pool();
			memory_pool::operator=(std::move(other));
			host_ptr = std::exchange(other.host_ptr, nullptr);
		}

		void* host_ptr = nullptr;
	};

	struct buffer_binding_args
	{
		VkBuffer buffer;
		VkDeviceSize frame_offset;
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	class device_memory
	{
	public:
		void init(device& owner);
		void terminate();

		device_memory() = default;

		device_memory(device_memory&& other) noexcept :
			owner(std::exchange(other.owner, nullptr)),
			mem_granularity(std::exchange(other.mem_granularity, 0)),
			mem_properties(std::exchange(other.mem_properties, {})),
			cmd_pool(std::exchange(other.cmd_pool, VK_NULL_HANDLE)), cmd_buffers(std::move(other.cmd_buffers)),
			device_in(std::move(other.device_in)), device_out(std::move(other.device_out)),
			vertex_pools(std::move(other.vertex_pools)), index_pools(std::move(other.index_pools)),
			uniform_pools(std::move(other.uniform_pools)), storage_pools(std::move(other.storage_pools)),
			d_vertex_pools(std::move(other.d_vertex_pools)), d_index_pools(std::move(other.d_index_pools)),
			d_uniform_pools(std::move(other.d_uniform_pools)), d_storage_pools(std::move(other.d_storage_pools)),
			vp_pending_copies(std::move(other.vp_pending_copies)), ip_pending_copies(std::move(other.ip_pending_copies)),
			up_pending_copies(std::move(other.up_pending_copies)), sp_pending_copies(std::move(other.sp_pending_copies))
		{}

		//void update_largest_slots();
		//void tick(); could maybe replace the above function and performa all updates and cleanup

		void flush_in(const std::uint8_t current_frame);
		//void flush_out();

		void map_ranges(const std::uint8_t current_frame);
		void unmap_ranges(const std::uint8_t current_frame);

		void synchronize(const std::uint8_t current_frame);

		memory_ref alloc_vertices(const VkDeviceSize size);
		memory_ref alloc_vertices(const void* data, const VkDeviceSize size);
		memory_ref alloc_indexes(const VkDeviceSize size);
		memory_ref alloc_indexes(const void* data, const VkDeviceSize size);
		memory_ref alloc_uniform(const VkDeviceSize size);
		memory_ref alloc_uniform(const void* data, const VkDeviceSize size);
		memory_ref alloc_storage(const VkDeviceSize size);
		memory_ref alloc_storage(const void* data, const VkDeviceSize size);

		memory_ref alloc_dynamic_vertices(const VkDeviceSize size);
		memory_ref alloc_dynamic_vertices(const void* data, const VkDeviceSize size);
		memory_ref alloc_dynamic_indexes(const VkDeviceSize size);
		memory_ref alloc_dynamic_indexes(const void* data, const VkDeviceSize size);
		memory_ref alloc_dynamic_uniform(const VkDeviceSize size);
		memory_ref alloc_dynamic_uniform(const void* data, const VkDeviceSize size);
		memory_ref alloc_dynamic_storage(const VkDeviceSize size);
		memory_ref alloc_dynamic_storage(const void* data, const VkDeviceSize size);
		
		//memory_ref alloc_texture();

		void submit_upload(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);

		void memcpy_vertices(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
		void memcpy_indexes(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
		void memcpy_uniform(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
		void memcpy_storage(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset = 0);

		void get_vertex_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;
		void get_index_map(const memory_ref & ref, void*** out_map_ptr, std::size_t * out_offset) noexcept;
		void get_uniform_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;
		void get_storage_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;

		buffer_binding_args get_binding_args(const mesh& object) noexcept;
		buffer_binding_args get_index_binding_args(const mesh& object) noexcept;
		buffer_binding_args get_binding_args(const uniform& uniform) noexcept;
		buffer_binding_args get_binding_args(const unmapped_uniform& uniform) noexcept;
		buffer_binding_args get_binding_args(const storage_array& array) noexcept;
		buffer_binding_args get_binding_args(const dynamic_storage_array& array) noexcept;
		buffer_binding_args get_binding_args(const storage_vector& vector) noexcept;
		buffer_binding_args get_binding_args(const dynamic_storage_vector& vector) noexcept;

		inline VkSemaphore get_device_in_semaphore(const std::uint8_t current_frame) { return device_in[current_frame].semaphore; }

		enum buffer_t : std::uint8_t
		{
			VERTEX,
			INDEX,
			UNIFORM,
			STORAGE,
			NONE,
		};

	private:
		
		typedef std::pair<std::uint32_t, std::uint32_t> copy_path_t;

		//not part of the engine API, so it's unnecessary to implement the std version of hash
		struct hash
		{
			inline std::size_t operator()(const copy_path_t& key) const noexcept
			{
				return static_cast<std::size_t>((static_cast<std::uint64_t>(key.first) << 32) | key.second);
			}
		};

		typedef std::unordered_map<copy_path_t, std::vector<VkBufferCopy>, hash> pending_copies_t;
		
		//there are some template functions used by other member functions, 
		//they are private so it's not a problem that they aren't defined in the header

		template<buffer_t BType>
		inline std::vector<memory_pool>& get_pools() 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<>
		inline std::vector<memory_pool>& get_pools<VERTEX>() noexcept { return vertex_pools; }
		template<>
		inline std::vector<memory_pool>& get_pools<INDEX>() noexcept { return index_pools; }
		template<>
		inline std::vector<memory_pool>& get_pools<UNIFORM>() noexcept { return uniform_pools; }
		template<>
		inline std::vector<memory_pool>& get_pools<STORAGE>() noexcept { return storage_pools; }

		template<buffer_t BType>
		inline std::vector<dynamic_memory_pool>& get_dynamic_pools() 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<>
		inline std::vector<dynamic_memory_pool>& get_dynamic_pools<VERTEX>() noexcept { return d_vertex_pools; }
		template<>
		inline std::vector<dynamic_memory_pool>& get_dynamic_pools<INDEX>() noexcept { return d_index_pools; }
		template<>
		inline std::vector<dynamic_memory_pool>& get_dynamic_pools<UNIFORM>() noexcept { return d_uniform_pools; }
		template<>
		inline std::vector<dynamic_memory_pool>& get_dynamic_pools<STORAGE>() noexcept { return d_storage_pools; }

		template<buffer_t BType>
		inline pending_copies_t& get_pending_copies(std::uint8_t current_frame) 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<>
		inline pending_copies_t& get_pending_copies<VERTEX>(std::uint8_t current_frame) noexcept
		{ return vp_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& get_pending_copies<INDEX>(std::uint8_t current_frame) noexcept
		{ return ip_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& get_pending_copies<UNIFORM>(std::uint8_t current_frame) noexcept
		{ return up_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& get_pending_copies<STORAGE>(std::uint8_t current_frame) noexcept
		{ return sp_pending_copies[current_frame]; }

		std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);

		void alloc_buffer(VkDeviceMemory& memory, VkBuffer& buffer, VkDeviceSize size, 
			VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		template<buffer_t BType, VkMemoryPropertyFlags MFlags>
		void alloc_slot(std::uint32_t* out_pool_idx, std::uint32_t* out_slot_idx, VkDeviceSize size);

		template<buffer_t BType>
		void submit_upload(std::uint32_t pool_idx, VkDeviceSize offset, const void* data, const VkDeviceSize size);

		template<buffer_t BType>
		void memcpy(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);

		template<buffer_t BType, VkMemoryPropertyFlags MFlags>
		memory_ref alloc(const VkDeviceSize size);

		template<buffer_t BType, VkMemoryPropertyFlags MFlags>
		memory_ref alloc(const void* data, const VkDeviceSize size);

		template<buffer_t BType>
		void get_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;

		template<buffer_t BType>
		buffer_binding_args get_binding_args(const resource&) noexcept;
		template<buffer_t BType>
		buffer_binding_args get_dynamic_binding_args(const resource&) noexcept;
		
		inline void set_owner(device& new_owner) noexcept { owner = &new_owner; }

		device* owner = nullptr;

		VkDeviceSize mem_granularity = 0; //from physical device properties
		VkPhysicalDeviceMemoryProperties mem_properties = {};

		VkCommandPool cmd_pool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, max_frames_in_flight> cmd_buffers;

		std::array<transfer_memory, max_frames_in_flight> device_in; //TODO: move all transfer memories to a single buffer and change offset depending on frame
		std::array<transfer_memory, max_frames_in_flight> device_out;

		std::vector<memory_pool> vertex_pools;
		std::vector<memory_pool> index_pools;
		std::vector<memory_pool> uniform_pools;
		std::vector<memory_pool> storage_pools;
		std::vector<dynamic_memory_pool> d_vertex_pools;
		std::vector<dynamic_memory_pool> d_index_pools;
		std::vector<dynamic_memory_pool> d_uniform_pools;
		std::vector<dynamic_memory_pool> d_storage_pools;

		std::array<pending_copies_t, max_frames_in_flight> vp_pending_copies;
		std::array<pending_copies_t, max_frames_in_flight> ip_pending_copies;
		std::array<pending_copies_t, max_frames_in_flight> up_pending_copies;
		std::array<pending_copies_t, max_frames_in_flight> sp_pending_copies;

		friend class device;
	};
}
