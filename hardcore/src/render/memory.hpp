#pragma once

#include "render_core.hpp"
#include "memory_pool.hpp"

#include <render/memory_ref.hpp>
#include <render/resource.hpp>

namespace ENGINE_NAMESPACE
{
	// for operations such as allocations, which happen outside of device functions, memory needs access to some device
	// data, such as the VkDevice handle for operations like vkAllocateMemory
	struct random_access_device_data
	{
		VkDevice device; // the device handle should never change throughout memory's lifetime, so it can be a local copy
		const VkPhysicalDeviceLimits* limits;
		const std::uint8_t* current_frame;
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
		void init(VkPhysicalDevice physical_device, VkDevice device, std::uint32_t transfer_queue_idx, 
			const VkPhysicalDeviceLimits* limits, const std::uint8_t* current_frame);
		void terminate(VkDevice device, std::uint8_t current_frame);

		device_memory() = default;

		device_memory(device_memory&& other) noexcept :
			radd(std::exchange(other.radd, {})),
			heap_manager(std::move(other.heap_manager)),
			cmd_pool(std::exchange(other.cmd_pool, VK_NULL_HANDLE)), cmd_buffers(std::move(other.cmd_buffers)),
			device_in(std::move(other.device_in)), device_out(std::move(other.device_out)),
			vertex_pools(std::move(other.vertex_pools)), index_pools(std::move(other.index_pools)),
			uniform_pools(std::move(other.uniform_pools)), storage_pools(std::move(other.storage_pools)),
			d_vertex_pools(std::move(other.d_vertex_pools)), d_index_pools(std::move(other.d_index_pools)),
			d_uniform_pools(std::move(other.d_uniform_pools)), d_storage_pools(std::move(other.d_storage_pools)),
			vp_pending_copies(std::move(other.vp_pending_copies)), ip_pending_copies(std::move(other.ip_pending_copies)),
			up_pending_copies(std::move(other.up_pending_copies)), sp_pending_copies(std::move(other.sp_pending_copies))
		{}
		
		inline void update_refs(VkDevice device, const VkPhysicalDeviceLimits* limits, const std::uint8_t* current_frame) noexcept 
		{ radd.device = device, radd.limits = limits, radd.current_frame = current_frame; }

		//void update_largest_slots();
		//void tick(); could maybe replace the above function and performa all updates and cleanup

		bool flush_in(VkDevice device, VkQueue transfer_queue, std::uint8_t current_frame);
		//void flush_out();

		void map_ranges(VkDevice device, std::uint8_t current_frame);
		void unmap_ranges(VkDevice device, std::uint8_t current_frame);

		void synchronize(VkDevice device, std::uint8_t current_frame);

		inline VkSemaphore get_device_in_semaphore(std::uint8_t current_frame) { return device_in[current_frame].semaphore; }

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

		void vertex_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;
		void index_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;
		void uniform_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;
		void storage_map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;

		buffer_binding_args binding_args(const mesh& object) noexcept;
		buffer_binding_args index_binding_args(const mesh& object) noexcept;
		buffer_binding_args binding_args(const uniform& uniform) noexcept;
		buffer_binding_args binding_args(const unmapped_uniform& uniform) noexcept;
		buffer_binding_args binding_args(const storage_array& array) noexcept;
		buffer_binding_args binding_args(const dynamic_storage_array& array) noexcept;
		buffer_binding_args binding_args(const storage_vector& vector) noexcept;
		buffer_binding_args binding_args(const dynamic_storage_vector& vector) noexcept;

		enum buffer_t : std::uint8_t
		{
			VERTEX,
			INDEX,
			UNIFORM,
			STORAGE,
			UNIVERSAL, //writable
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
		inline std::vector<buffer_pool>& static_pools() 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<> inline std::vector<buffer_pool>& static_pools<VERTEX>() noexcept { return vertex_pools; }
		template<> inline std::vector<buffer_pool>& static_pools<INDEX>() noexcept { return index_pools; }
		template<> inline std::vector<buffer_pool>& static_pools<UNIFORM>() noexcept { return uniform_pools; }
		template<> inline std::vector<buffer_pool>& static_pools<STORAGE>() noexcept { return storage_pools; }
		template<> inline std::vector<buffer_pool>& static_pools<UNIVERSAL>() noexcept { return writable_pools; }

		template<buffer_t BType>
		inline std::vector<dynamic_buffer_pool>& dynamic_pools() 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<> inline std::vector<dynamic_buffer_pool>& dynamic_pools<VERTEX>() noexcept { return d_vertex_pools; }
		template<> inline std::vector<dynamic_buffer_pool>& dynamic_pools<INDEX>() noexcept { return d_index_pools; }
		template<> inline std::vector<dynamic_buffer_pool>& dynamic_pools<UNIFORM>() noexcept { return d_uniform_pools; }
		template<> inline std::vector<dynamic_buffer_pool>& dynamic_pools<STORAGE>() noexcept { return d_storage_pools; }

		template<buffer_t BType>
		inline pending_copies_t& pending_copies(std::uint8_t current_frame) 
		{ static_assert(force_eval<BType>::value, "Unimplemented buffer type"); }
		template<>
		inline pending_copies_t& pending_copies<VERTEX>(std::uint8_t current_frame) noexcept
		{ return vp_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& pending_copies<INDEX>(std::uint8_t current_frame) noexcept
		{ return ip_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& pending_copies<UNIFORM>(std::uint8_t current_frame) noexcept
		{ return up_pending_copies[current_frame]; }
		template<>
		inline pending_copies_t& pending_copies<STORAGE>(std::uint8_t current_frame) noexcept
		{ return sp_pending_copies[current_frame]; }

		template<buffer_t BType> inline VkDeviceSize offset_alignment() const noexcept
		{ return 0; }
		template<> inline VkDeviceSize offset_alignment<device_memory::UNIFORM>() const noexcept
		{ return radd.limits->minUniformBufferOffsetAlignment; }
		template<> inline VkDeviceSize offset_alignment<device_memory::STORAGE>() const noexcept
		{ return radd.limits->minStorageBufferOffsetAlignment; }
		template<> inline VkDeviceSize offset_alignment<device_memory::UNIVERSAL>() const noexcept
		{ return radd.limits->minStorageBufferOffsetAlignment; }

		template<buffer_t BType, bool Dynamic>
		memory_ref alloc_buffer(VkDeviceSize size);

		template<buffer_t BType, bool Dynamic>
		memory_ref alloc_buffer(const void* data, VkDeviceSize size);

		template<buffer_t BType>
		void submit_upload(std::uint32_t pool_idx, VkDeviceSize offset, const void* data, const VkDeviceSize size);

		template<buffer_t BType>
		void memcpy(const memory_ref& ref, const void* data, const VkDeviceSize size, const VkDeviceSize offset);

		template<buffer_t BType>
		void map(const memory_ref& ref, void*** out_map_ptr, std::size_t* out_offset) noexcept;

		template<buffer_t BType>
		buffer_binding_args binding_args(const resource&) noexcept;
		template<buffer_t BType>
		buffer_binding_args dynamic_binding_args(const resource&) noexcept;

		template<bool Dynamic>
		memory_ref alloc_texture(VkDeviceSize size);

		random_access_device_data radd;

		device_heap_manager heap_manager;

		VkCommandPool cmd_pool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, max_frames_in_flight> cmd_buffers;

		std::array<transfer_memory, max_frames_in_flight> device_in; //TODO: move all transfer memories to a single buffer and change offset depending on frame
		std::array<transfer_memory, max_frames_in_flight> device_out;

		// "read-only" resources

		std::vector<buffer_pool> vertex_pools;
		std::vector<buffer_pool> index_pools;
		std::vector<buffer_pool> uniform_pools;
		std::vector<buffer_pool> storage_pools;

		// "writable" resources (for a resource to be writable in shaders, it has to be a storage buffer,
		// which I am assuming is the slowest type of buffer, so instead of having a variable for each type
		// of buffer, there is only one tagged with all types)

		std::vector<buffer_pool> writable_pools;

		// resources directly writable by the cpu

		std::vector<dynamic_buffer_pool> d_vertex_pools;
		std::vector<dynamic_buffer_pool> d_index_pools;
		std::vector<dynamic_buffer_pool> d_uniform_pools;
		std::vector<dynamic_buffer_pool> d_storage_pools;

		std::vector<std::vector<texture_pool>> texture_pools;

		std::array<pending_copies_t, max_frames_in_flight> vp_pending_copies; //TODO only 1 pending copies is needed
		std::array<pending_copies_t, max_frames_in_flight> ip_pending_copies;
		std::array<pending_copies_t, max_frames_in_flight> up_pending_copies;
		std::array<pending_copies_t, max_frames_in_flight> sp_pending_copies;
	};
}
