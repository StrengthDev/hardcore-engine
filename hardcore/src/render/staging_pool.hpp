#pragma once

#include "render_core.hpp"
#include "device_heap_manager.hpp"

namespace ENGINE_NAMESPACE
{
	class staging_pool;

	template<class T>
	concept StagingPoolType = std::is_base_of<staging_pool, T>::value && !std::is_same<staging_pool, T>::value;

	class staging_pool
	{
	public:
		void free(VkDevice device, device_heap_manager& heap_manager);

		void map(VkDevice device, u8 current_frame);
		void unmap(VkDevice device);

		inline VkDeviceSize capacity() const noexcept { return m_size; }
		inline VkDeviceSize size() const noexcept { return m_pending_size; }

		template<StagingPoolType T>
		static void push_flush_ranges(std::vector<VkMappedMemoryRange>& ranges, u8 current_frame, 
			const std::vector<T>& pools) // this function has to be a template as the vector contains values
		{
			for (const auto& pool : pools)
			{
				if (!pool.m_pending_size)
					continue;

				ranges.push_back(pool.mapped_range(current_frame));
			}
		}

	protected:
		staging_pool() = default;
		staging_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size, 
			VkBufferUsageFlags usage, device_heap_manager::heap heap);
		~staging_pool();

		staging_pool(const staging_pool&) = delete;
		staging_pool& operator=(const staging_pool&) = delete;

		staging_pool(staging_pool&& other) noexcept :
			m_memory(std::exchange(other.m_memory, VK_NULL_HANDLE)),
			m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
			m_host_ptr(std::exchange(other.m_host_ptr, nullptr)),
			m_size(std::exchange(other.m_size, 0)),
			m_pending_size(std::exchange(other.m_pending_size, 0))
		{}

		inline staging_pool& operator=(staging_pool&& other) noexcept
		{
			INTERNAL_ASSERT(!m_host_ptr, "Other staging pool not unmapped");
			INTERNAL_ASSERT(m_memory == VK_NULL_HANDLE, "Other staging pool not freed");

			m_memory = std::exchange(other.m_memory, VK_NULL_HANDLE);
			m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
			m_host_ptr = std::exchange(other.m_host_ptr, nullptr);
			m_size = std::exchange(other.m_size, 0);
			m_pending_size = std::exchange(other.m_pending_size, 0);
			return *this;
		}

		inline VkMappedMemoryRange mapped_range(u8 current_frame) const noexcept
		{
			return { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .pNext = nullptr, .memory = m_memory,
				.offset = m_size * current_frame, .size = m_size };
		}


		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		void* m_host_ptr = nullptr;
		VkDeviceSize m_size;
		VkDeviceSize m_pending_size;
	};

	class upload_pool : public staging_pool
	{
	public:
		upload_pool() = default;
		upload_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size) :
			staging_pool(device, heap_manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device_heap_manager::heap::UPLOAD)
		{}

		inline bool has_batch(u8 pool_type, std::size_t pool_idx) const
		{
			return m_pending_copies.contains({ pool_type, pool_idx });
		}

		void create_batch(u8 pool_type, std::size_t pool_idx, VkBuffer buffer);
		void submit(u8 pool_type, std::size_t pool_idx, const void* data, VkDeviceSize size, VkDeviceSize offset);

		void record_and_clear(VkCommandBuffer& buffer);

		//static void push_flush_ranges(std::vector<VkMappedMemoryRange>& ranges, u8 current_frame,
		//	const std::vector<upload_pool>& pools);

	private:
		typedef std::pair<u8, std::size_t> dst_pool; //<pool_type, pool_idx>

		//not part of the engine API, so it's unnecessary to implement the std version of hash
		struct hash
		{
			inline std::size_t operator()(const dst_pool& key) const noexcept
			{
				return static_cast<std::size_t>((static_cast<u64>(key.second) << (sizeof(key.first) * 8)) ^ key.first);
			}
		};

		struct dst_data
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			std::vector<VkBufferCopy> regions;
		};

		std::unordered_map<dst_pool, dst_data, hash> m_pending_copies;
	};

	class texture_upload_pool : public staging_pool
	{
	public:
		texture_upload_pool() = default;
		texture_upload_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size) :
			staging_pool(device, heap_manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, device_heap_manager::heap::UPLOAD)
		{}

		void buffer_image_copy(const void* data, VkDeviceSize size, 
			VkImage image, VkImageLayout layout, VkExtent3D image_dims);
		//void image_copy();
		//void depth_stencil_copy();
		//void image_blit();
		//void image_resolve();

		void record_and_clear_transfer(VkCommandBuffer& buffer, u32 queue_idx);
		void record_and_clear_graphics(VkCommandBuffer& buffer, u32 queue_idx);

	private:
		enum conversion_type : u8
		{
			// transfer queue compatible

			BUFFER_TO_IMAGE = 0,
			TEXEL_ONLY_CONVERSION,

			// graphics queue only

			BUFFER_TO_DEPTH_OR_STENCIL,
			FULL_CONVERSION,
			MULTISAMPLED_TO_SIMPLE,
			MULTISAMPLED_TO_MULTISAMPLED,

			INVALID = 0xff
		};

		struct transfer_data
		{
			union
			{
				VkImage image;
				VkBuffer buffer;
			} src;
			VkImageLayout src_layout_0 = VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout src_layout_1 = VK_IMAGE_LAYOUT_UNDEFINED;

			VkImage dst_image = VK_NULL_HANDLE;
			VkImageLayout dst_layout_0 = VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout dst_layout_1 = VK_IMAGE_LAYOUT_UNDEFINED;

			conversion_type conversion = INVALID;
			VkImageAspectFlags aspect = 0;
			void* regions = nullptr;
			u32 region_count = 0;
			VkFilter filter = VK_FILTER_NEAREST;
		};

		std::vector<transfer_data> m_pending_copies_transfer;
		std::vector<transfer_data> m_pending_copies_graphics;
	};
}
