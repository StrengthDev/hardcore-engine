#pragma once

#include "render_core.hpp"

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
		memory_pool(VkDevice device, VkDeviceSize size);
		~memory_pool();

		void free(VkDevice device);

		bool search(VkDeviceSize size, VkDeviceSize alignment, 
			std::uint32_t* out_slot_idx, VkDeviceSize* out_size_needed);
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
		}

		inline VkDeviceMemory& memory() noexcept { return _memory; }
		inline VkDeviceSize size() const noexcept { return _size; }
		inline const memory_slot& operator[](std::uint32_t idx) const noexcept { return slots[idx]; }

	protected:
		virtual void realloc_slots(std::uint32_t new_count) = 0;
		virtual void move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count) = 0;

		VkDeviceMemory _memory = VK_NULL_HANDLE;
		VkDeviceSize _size = 0;

		memory_slot* slots = nullptr;
		std::uint32_t n_slots = 0;
		std::uint32_t largest_free_slot = 0;
	};

	class buffer_pool : public memory_pool
	{
	public:
		buffer_pool() = default;
		buffer_pool(VkDevice device, VkDeviceSize size) : memory_pool(device, size) {}

		void buffer_pool::free(VkDevice device);

		buffer_pool(buffer_pool&& other) noexcept :
			memory_pool(std::move(other)),
			_buffer(std::exchange(other._buffer, VK_NULL_HANDLE))
		{ }

		inline buffer_pool& operator=(buffer_pool&& other) noexcept
		{
			memory_pool::operator=(std::move(other));
			_buffer = std::exchange(other._buffer, VK_NULL_HANDLE);
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
		dynamic_buffer_pool(VkDevice device, VkDeviceSize size) : buffer_pool(device, size), _host_ptr(nullptr) {}

		dynamic_buffer_pool(dynamic_buffer_pool&& other) noexcept :
			buffer_pool(std::move(other)),
			_host_ptr(std::exchange(other._host_ptr, nullptr))
		{ }

		inline dynamic_buffer_pool& operator=(dynamic_buffer_pool&& other) noexcept
		{
			buffer_pool::operator=(std::move(other));
			_host_ptr = std::exchange(other._host_ptr, nullptr);
		}

		void map(VkDevice device, std::uint8_t current_frame);
		void unmap(VkDevice device);

		inline void*& host_ptr() noexcept { return _host_ptr; }

	private:
		void* _host_ptr = nullptr;
	};

	struct texture_slot
	{
		VkImage image;
		VkImageView view;
	};

	struct writable_texture_slot
	{
		VkImage image;
		std::array<VkImageView, max_frames_in_flight> views;
	};

	class texture_pool : public memory_pool
	{
	public:

	protected:
		void realloc_slots(std::uint32_t new_count) override;
		void move_slots(std::uint32_t dst, std::uint32_t src, std::uint32_t count) override;

	private:
		texture_slot* texture_slots = nullptr;
	};

	constexpr VkDeviceSize alignment_pad(VkDeviceSize offset, VkDeviceSize alignment)
	{
		if (alignment)
			return (alignment - (offset % alignment)) % alignment;
		else
			return 0;
	}

	constexpr VkDeviceSize aligned_offset(VkDeviceSize offset, VkDeviceSize alignment)
	{
		return offset + alignment_pad(offset, alignment);
	}

}
