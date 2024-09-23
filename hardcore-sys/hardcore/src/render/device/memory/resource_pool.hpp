#pragma once

#include <memory>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <util/result.hpp>
#include <util/uncopyable.hpp>

#include "heap_manager.hpp"
#include "allocation_pool.hpp"

namespace hc::render::device::memory {
    enum class PoolResult : u8 {
        Success = 0,
        OutOfHostMemory,
        OutOfDeviceMemory,
        UnsupportedHeap,
        MapFailure,
    };

    class BufferPool : public AllocationPool<NullSlot> {
    public:
        BufferPool() = default;

        [[nodiscard]]
        static Result<BufferPool, PoolResult> create(const VolkDeviceTable &fn_table, VkDevice device,
                                                     HeapManager &heap_manager, VkDeviceSize size,
                                                     VkBufferUsageFlags usage);

        void free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept;

        inline VkBuffer &handle() noexcept { return this->buffer; }

    protected:
        BufferPool(VkDeviceMemory memory, VkDeviceSize size, bool per_frame_allocation) :
                AllocationPool<NullSlot>(memory, size, per_frame_allocation) {}

        ExternalHandle<VkBuffer, VK_NULL_HANDLE> buffer;
    };

    class DynamicBufferPool : public BufferPool {
    public:
        DynamicBufferPool() = default;

        [[nodiscard]]
        static Result<DynamicBufferPool, PoolResult> create(const VolkDeviceTable &fn_table, VkDevice device,
                                                            HeapManager &heap_manager, VkDeviceSize size,
                                                            VkBufferUsageFlags usage);

        ~DynamicBufferPool();

        DynamicBufferPool(DynamicBufferPool &&) noexcept = default;

        DynamicBufferPool &operator=(DynamicBufferPool &&) = default;

        [[nodiscard]] PoolResult map(const VolkDeviceTable &fn_table, VkDevice device, u8 frame_mod);

        void unmap(const VolkDeviceTable &fn_table, VkDevice device);

        inline void **host_ptr() const noexcept { return this->mapped_host_ptr.get(); }

        inline VkMappedMemoryRange mapped_range(u8 frame_mod) const noexcept {
            return {
                    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .pNext = nullptr,
                    .memory = this->memory,
                    .offset = this->total_capacity * frame_mod,
                    .size = this->total_capacity
            };
        }

    private:
        DynamicBufferPool(VkDeviceMemory memory, VkDeviceSize size) : BufferPool(memory, size, true) {}

        // Use unique pointer so that the location never changes, even if the pool is moved. Resource handles will have
        // a pointer to this pointer.
        // Unique pointers cannot be copied, so there is no need to wrap this in an `Uncopyable`.
        std::unique_ptr<void *> mapped_host_ptr;
    };
/*
    struct texture_slot {
        VkImage image;
        VkImageView view;

        VkDeviceSize size;
        VkExtent3D dims;
        VkImageLayout layout; // Layout of the image at the start of the frame
    };

    texture_slot
    create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements &out_memory_requirements);

    class texture_pool : public AllocationPool<TextureSlot> {
    public:
        texture_pool() = default;

        texture_pool(VkDevice device, HeapManager &heap_manager, VkDeviceSize size,
                     u32 memory_type_bits, Heap preferred_heap);

        void free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept;

        bool search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
                    u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const;

        void fill_slot(VkDevice device, TextureSlot &&tex, u32 slot_idx, VkDeviceSize size, VkDeviceSize alignment);

        const TextureSlot &tex_at(u32 idx) const noexcept { return this->extra_slots[idx]; }

    private:
        u32 m_memory_type_idx = std::numeric_limits<u32>::max();
    };
*/
}
