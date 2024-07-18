#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/util.hpp>

#include "heap_manager.hpp"

namespace hc::device::memory {
    struct memory_slot {
        VkDeviceSize offset; // Relative to previous slot
        VkDeviceSize size;
        bool in_use;
    };

    class resource_pool {
    public:
        resource_pool(const resource_pool &) = delete;

        resource_pool &operator=(const resource_pool &) = delete;

        bool search(VkDeviceSize size, VkDeviceSize alignment,
                    u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const;

        void fill_slot(u32 slot_idx, VkDeviceSize size);

        // TODO cleanup function to remove excessive amount of empty slots maybe
        u32 find_slot(VkDeviceSize offset) const noexcept;

        inline VkDeviceSize size() const noexcept { return this->capacity; }

        inline const memory_slot &operator[](u32 idx) const noexcept { return this->slots[idx]; }

    protected:
        resource_pool() = default;

        ~resource_pool();

        void free_memory(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept;

        explicit resource_pool(VkDeviceSize size, bool per_frame_allocation = false);

        resource_pool(resource_pool &&other) noexcept:
                memory(std::exchange(other.memory, VK_NULL_HANDLE)),
                capacity(std::exchange(other.capacity, 0)),
                slots(std::move(other.slots)),
                largest_free_slot(std::exchange(other.largest_free_slot, 0)) {}

        inline resource_pool &operator=(resource_pool &&other) noexcept {
            HC_ASSERT(other.memory == VK_NULL_HANDLE, "Other resource pool not freed");

            this->memory = std::exchange(other.memory, VK_NULL_HANDLE);
            this->capacity = std::exchange(other.capacity, 0);
            this->slots = std::move(other.slots);
            this->largest_free_slot = std::exchange(other.largest_free_slot, 0);
            return *this;
        }

        virtual void push_back_empty() = 0;

        virtual void move_slots(u32 dst, u32 src, u32 count) = 0;

        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize capacity = 0;

        std::vector<memory_slot> slots;

        mutable u32 largest_free_slot = 0;
        // TODO is this needed as a member variable?
        bool per_frame_allocation = false;
    };

    class buffer_pool : public resource_pool {
    public:
        buffer_pool() = default;

        buffer_pool(VkDevice device, HeapManager &heap_manager, VkDeviceSize size, VkBufferUsageFlags usage) :
                buffer_pool(device, heap_manager, size, usage, Heap::Main, false) {}

        void free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept;

        buffer_pool(buffer_pool &&other) noexcept:
                resource_pool(std::move(other)),
                buffer(std::exchange(other.buffer, VK_NULL_HANDLE)) {}

        inline buffer_pool &operator=(buffer_pool &&other) noexcept {
            this->buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
            resource_pool::operator=(std::move(other));
            return *this;
        }

        inline VkBuffer &handle() noexcept { return this->buffer; }

    protected:
        buffer_pool(VkDevice device, HeapManager &heap_manager,
                    VkDeviceSize size, VkBufferUsageFlags usage, Heap heap,
                    bool per_frame_allocation);

        void push_back_empty() override;

        void move_slots(u32 dst, u32 src, u32 count) override;

        VkBuffer buffer = VK_NULL_HANDLE;
    };

    class dynamic_buffer_pool : public buffer_pool {
    public:
        dynamic_buffer_pool() = default;

        dynamic_buffer_pool(VkDevice device, HeapManager &heap_manager,
                            VkDeviceSize size, VkBufferUsageFlags usage) :
                buffer_pool(device, heap_manager, size, usage, Heap::Dynamic, true),
                mapped_host_ptr(nullptr) {}

        dynamic_buffer_pool(dynamic_buffer_pool &&other) noexcept:
                buffer_pool(std::move(other)),
                mapped_host_ptr(std::exchange(other.mapped_host_ptr, nullptr)) {}

        inline dynamic_buffer_pool &operator=(dynamic_buffer_pool &&other) noexcept {
            this->mapped_host_ptr = std::exchange(other.mapped_host_ptr, nullptr);
            buffer_pool::operator=(std::move(other));
            return *this;
        }

        void map(VkDevice device, u8 current_frame);

        void unmap(VkDevice device);

        static void push_flush_ranges(std::vector<VkMappedMemoryRange> &ranges, u8 current_frame,
                                      const std::vector<dynamic_buffer_pool> &pools);

        inline void *host_ptr() const noexcept { return this->mapped_host_ptr; }

    private:
        inline VkMappedMemoryRange mapped_range(u8 current_frame) const noexcept {
            return {.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .pNext = nullptr, .memory = this->memory,
                    .offset = this->capacity * current_frame, .size = this->capacity};
        }

        void *mapped_host_ptr = nullptr;
    };

    struct texture_slot {
        VkImage image;
        VkImageView view;

        VkDeviceSize size;
        VkExtent3D dims;
        VkImageLayout layout; // Layout of the image at the start of the frame
    };

    texture_slot
    create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements &out_memory_requirements);

    class texture_pool : public resource_pool {
    public:
        texture_pool() = default;

        texture_pool(VkDevice device, HeapManager &heap_manager, VkDeviceSize size,
                     u32 memory_type_bits, Heap preferred_heap);

        void free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept;

        bool search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
                    u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const;

        void fill_slot(VkDevice device, texture_slot &&tex, u32 slot_idx, VkDeviceSize size, VkDeviceSize alignment);

        const texture_slot &tex_at(u32 idx) const noexcept { return this->m_texture_slots[idx]; }

    protected:
        void push_back_empty() override;

        void move_slots(u32 dst, u32 src, u32 count) override;

    private:
        u32 m_memory_type_idx = std::numeric_limits<u32>::max();
        std::vector<texture_slot> m_texture_slots;
    };

    /**
     * @brief This function calculates the amount of bytes necessary to add to the given offset, in order for it to be
     * aligned to the given alignment.
     *
     * @param offset Starting offset.
     * @param alignment Target memory alignment.
     * @return Number of padding bytes.
    */
    constexpr VkDeviceSize alignment_pad(VkDeviceSize offset, VkDeviceSize alignment) {
        if (alignment)
            return (alignment - (offset % alignment)) % alignment;
        else
            return 0;
    }

    /**
     * @brief This function calculates a new offset, larger than the given offset, aligned to alignment.
     *
     * @param offset Starting offset.
     * @param alignment Target memory alignment.
     * @return New aligned offset.
    */
    constexpr VkDeviceSize aligned_offset(VkDeviceSize offset, VkDeviceSize alignment) {
        return offset + alignment_pad(offset, alignment);
    }
}
