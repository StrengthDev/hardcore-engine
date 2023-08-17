#pragma once

#include "render_core.hpp"
#include "device_heap_manager.hpp"

namespace ENGINE_NAMESPACE
{
	struct memory_slot
	{
		bool in_use;
		VkDeviceSize offset; //relative to previous slot
		VkDeviceSize size;
	};

	class resource_pool
	{
	public:
		void free(VkDevice device, device_heap_manager& heap_manager);

		bool search(VkDeviceSize size, VkDeviceSize alignment, 
			u32& out_slot_idx, VkDeviceSize& out_size_needed, VkDeviceSize& out_offset) const;
		void fill_slot(u32 slot_idx, VkDeviceSize size);
		// TODO cleanup function to remove excessive amount of empty slots maybe
		u32 find_slot(VkDeviceSize offset) const noexcept;

		inline VkDeviceSize size() const noexcept { return m_size; }
		inline const memory_slot& operator[](u32 idx) const noexcept { return m_slots[idx]; }

	protected:
		resource_pool() = default;
		~resource_pool();

		resource_pool(VkDeviceSize size, bool per_frame_allocation = false);

		resource_pool(const resource_pool&) = delete;
		resource_pool& operator=(const resource_pool&) = delete;

		resource_pool(resource_pool&& other) noexcept :
			m_memory(std::exchange(other.m_memory, VK_NULL_HANDLE)),
			m_size(std::exchange(other.m_size, 0)),
			m_slots(std::move(other.m_slots)),
			m_largest_free_slot(std::exchange(other.m_largest_free_slot, 0))
		{ }

		inline resource_pool& operator=(resource_pool&& other) noexcept
		{
			INTERNAL_ASSERT(other.m_memory == VK_NULL_HANDLE, "Other resource pool not freed");
			
			m_memory = std::exchange(other.m_memory, VK_NULL_HANDLE);
			m_size = std::exchange(other.m_size, 0);
			m_slots = std::move(other.m_slots);
			m_largest_free_slot = std::exchange(other.m_largest_free_slot, 0);
			return *this;
		}

		virtual void push_back_empty() = 0;
		virtual void move_slots(u32 dst, u32 src, u32 count) = 0;

		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkDeviceSize m_size = 0;

		std::vector<memory_slot> m_slots;

		mutable u32 m_largest_free_slot = 0;
		//TODO is this needed as a member variable?
		bool m_per_frame_allocation = false;
	};

	class buffer_pool : public resource_pool
	{
	public:
		buffer_pool() = default;
		buffer_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size, VkBufferUsageFlags usage) :
			buffer_pool(device, heap_manager, size, usage, device_heap_manager::heap::MAIN, false)
		{ }

		void free(VkDevice device, device_heap_manager& heap_manager);

		buffer_pool(buffer_pool&& other) noexcept :
			resource_pool(std::move(other)),
			m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE))
		{ }

		inline buffer_pool& operator=(buffer_pool&& other) noexcept
		{
			m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
			resource_pool::operator=(std::move(other));
			return *this;
		}

		inline VkBuffer& buffer() noexcept { return m_buffer; }

	protected:
		buffer_pool(VkDevice device, device_heap_manager& heap_manager,
			VkDeviceSize size, VkBufferUsageFlags usage, device_heap_manager::heap heap,
			bool per_frame_allocation);

		void push_back_empty() override;
		void move_slots(u32 dst, u32 src, u32 count) override;

		VkBuffer m_buffer = VK_NULL_HANDLE;
	};

	class dynamic_buffer_pool : public buffer_pool
	{
	public:
		dynamic_buffer_pool() = default;
		dynamic_buffer_pool(VkDevice device, device_heap_manager& heap_manager,
			VkDeviceSize size, VkBufferUsageFlags usage) :
			buffer_pool(device, heap_manager, size, usage, device_heap_manager::heap::DYNAMIC, true), m_host_ptr(nullptr)
		{ }

		dynamic_buffer_pool(dynamic_buffer_pool&& other) noexcept :
			buffer_pool(std::move(other)),
			m_host_ptr(std::exchange(other.m_host_ptr, nullptr))
		{ }

		inline dynamic_buffer_pool& operator=(dynamic_buffer_pool&& other) noexcept
		{
			m_host_ptr = std::exchange(other.m_host_ptr, nullptr);
			buffer_pool::operator=(std::move(other));
			return *this;
		}

		void map(VkDevice device, u8 current_frame);
		void unmap(VkDevice device);

		static void push_flush_ranges(std::vector<VkMappedMemoryRange>& ranges, u8 current_frame,
			const std::vector<dynamic_buffer_pool>& pools);

		inline void*& host_ptr() noexcept { return m_host_ptr; }

	private:
		inline VkMappedMemoryRange mapped_range(u8 current_frame) const noexcept
		{
			return { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .pNext = nullptr, .memory = m_memory, 
				.offset = m_size * current_frame, .size = m_size };
		}

		void* m_host_ptr = nullptr;
	};

	struct texture_slot
	{
		VkImage image;
		VkImageView view;

		VkDeviceSize size;
		VkExtent3D dims;
		VkImageLayout layout; // layout of the image at the start of the frame
	};

	texture_slot create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements& out_memory_requirements);

	class texture_pool : public resource_pool //maybe use templated texture_memory_pool
	{
	public:
		texture_pool() = default;
		texture_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size,
			u32 memory_type_bits, device_heap_manager::heap preferred_heap);

		void free(VkDevice device, device_heap_manager& heap_manager);

		bool search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
			u32& out_slot_idx, VkDeviceSize& out_size_needed, VkDeviceSize& out_offset) const;
		void fill_slot(VkDevice device, texture_slot&& tex, u32 slot_idx, VkDeviceSize size, VkDeviceSize alignment);

		const texture_slot& tex_at(u32 idx) const noexcept { return m_texture_slots[idx]; }

	protected:
		void push_back_empty() override;
		void move_slots(u32 dst, u32 src, u32 count) override;

	private:
		u32 m_memory_type_idx = std::numeric_limits<u32>::max();
		std::vector<texture_slot> m_texture_slots;
	};

	/**
	 * @brief This function calculates the amount of bytes necessary to add to the given offset, in order for it to be
	 * aligned to the given alignment
	 * @param offset Starting offset
	 * @param alignment Target memory alignment
	 * @return Number of padding bytes
	*/
	constexpr VkDeviceSize alignment_pad(VkDeviceSize offset, VkDeviceSize alignment)
	{
		if (alignment)
			return (alignment - (offset % alignment)) % alignment;
		else
			return 0;
	}

	/**
	 * @brief This function calculates a new offset, larger than the given offset, aligned to alignment
	 * @param offset Starting offset
	 * @param alignment Target memory alignment
	 * @return New aligned offset
	*/
	constexpr VkDeviceSize aligned_offset(VkDeviceSize offset, VkDeviceSize alignment)
	{
		return offset + alignment_pad(offset, alignment);
	}
}
