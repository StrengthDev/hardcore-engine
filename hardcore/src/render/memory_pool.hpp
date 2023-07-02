#pragma once

#include "render_core.hpp"
#include "device_heap_manager.hpp"

namespace ENGINE_NAMESPACE
{
	struct memory_slot
	{
		bool in_use;
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	class memory_pool
	{
	public:
		memory_pool() = default;
		~memory_pool();

		void free(VkDevice device, device_heap_manager& heap_manager);

		bool search(VkDeviceSize size, VkDeviceSize alignment, 
			std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed) const;
		void fill_slot(std::uint32_t slot_idx, VkDeviceSize size);

		memory_pool(const memory_pool&) = delete;
		memory_pool& operator=(const memory_pool&) = delete;

		memory_pool(memory_pool&& other) noexcept :
			_memory(std::exchange(other._memory, VK_NULL_HANDLE)),
			_size(std::exchange(other._size, 0)),
			slots(std::exchange(other.slots, nullptr)),
			n_slots(std::exchange(other.n_slots, 0)),
			largest_free_slot(std::exchange(other.largest_free_slot, 0))
		{ }

		inline memory_pool& operator=(memory_pool&& other) noexcept
		{
			this->~memory_pool();
			_memory = std::exchange(other._memory, VK_NULL_HANDLE);
			_size = std::exchange(other._size, 0);
			slots = std::exchange(other.slots, nullptr);
			n_slots = std::exchange(other.n_slots, 0);
			largest_free_slot = std::exchange(other.largest_free_slot, 0);
			return *this;
		}

		//inline VkDeviceMemory& memory() noexcept { return _memory; }
		inline VkDeviceSize size() const noexcept { return _size; }
		inline const memory_slot& operator[](std::uint32_t idx) const noexcept { return slots[idx]; }

	protected:
		memory_pool(VkDeviceSize size, bool per_frame_allocation = false);

		virtual void realloc_slots(std::uint32_t new_count) = 0;
		virtual void move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count) = 0;

		VkDeviceMemory _memory = VK_NULL_HANDLE;
		VkDeviceSize _size = 0;

		memory_slot* slots = nullptr;
		std::uint32_t n_slots = 0;

		mutable std::uint32_t largest_free_slot = 0;

		bool per_frame_allocation = false;
	};

	class buffer_pool : public memory_pool
	{
	public:
		buffer_pool() = default;
		buffer_pool(VkDevice device, device_heap_manager& heap_manager,
			VkDeviceSize size, VkBufferUsageFlags usage, device_heap_manager::heap h,
			bool per_frame_allocation = false);

		void buffer_pool::free(VkDevice device, device_heap_manager& heap_manager);

		buffer_pool(buffer_pool&& other) noexcept :
			memory_pool(std::move(other)),
			_buffer(std::exchange(other._buffer, VK_NULL_HANDLE))
		{ }

		inline buffer_pool& operator=(buffer_pool&& other) noexcept
		{
			memory_pool::operator=(std::move(other));
			_buffer = std::exchange(other._buffer, VK_NULL_HANDLE);
			return *this;
		}

		inline VkBuffer& buffer() noexcept { return _buffer; }

	protected:
		void realloc_slots(std::uint32_t new_count) override;
		void move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count) override;

		VkBuffer _buffer = VK_NULL_HANDLE;
	};

	class dynamic_buffer_pool : public buffer_pool
	{
	public:
		dynamic_buffer_pool() = default;
		dynamic_buffer_pool(VkDevice device, device_heap_manager& heap_manager,
			VkDeviceSize size, VkBufferUsageFlags usage, device_heap_manager::heap h) :
			buffer_pool(device, heap_manager, size, usage, h, true), _host_ptr(nullptr)
		{ }

		dynamic_buffer_pool(dynamic_buffer_pool&& other) noexcept :
			buffer_pool(std::move(other)),
			_host_ptr(std::exchange(other._host_ptr, nullptr))
		{ }

		inline dynamic_buffer_pool& operator=(dynamic_buffer_pool&& other) noexcept
		{
			buffer_pool::operator=(std::move(other));
			_host_ptr = std::exchange(other._host_ptr, nullptr);
			return *this;
		}

		void map(VkDevice device, std::uint8_t current_frame);
		void unmap(VkDevice device);

		static void flush(VkDevice device, std::uint8_t current_frame, 
			const std::vector<const std::vector<dynamic_buffer_pool>*>& pools); //TODO the amount of vectors is known at compile time

		inline void*& host_ptr() noexcept { return _host_ptr; }

	private:
		inline VkMappedMemoryRange mapped_range(std::uint8_t current_frame) const noexcept
		{
			return { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, _memory, _size * current_frame, _size };
		}

		void* _host_ptr = nullptr;
	};

	struct texture
	{
		VkImage image;
		VkImageView view;
	};

	texture create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements* out_memory_requirements);

	class texture_pool : public memory_pool //maybe use templated texture_memory_pool
	{
	public:
		texture_pool() = default;
		texture_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size,
			std::uint32_t memory_type_bits, VkMemoryPropertyFlags heap_properties);

		bool search(VkDeviceSize size, VkDeviceSize alignment, std::uint32_t memory_type_bits,
			std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed);
		void fill_slot(VkDevice device, texture&& tex, std::uint32_t slot_idx, VkDeviceSize size, VkDeviceSize alignment);

	protected:
		void realloc_slots(std::uint32_t new_count) override;
		void move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count) override;

	private:
		std::uint32_t memory_type_idx = std::numeric_limits<std::uint32_t>::max();
		texture* texture_slots = nullptr;
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
