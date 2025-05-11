#pragma once

#include "heap_manager.hpp"
#include "allocation_pool.hpp"

#include <util/result.hpp>
#include <util/uncopyable.hpp>

#include <expected>
#include <memory>

namespace hc::render::device::memory {
    enum class PoolResult : u8 {
        Success = 0,
        OutOfHostMemory,
        OutOfDeviceMemory,
        UnsupportedHeap,
        MapFailure,
        NotEnoughSpace,
    };

    class BufferPool : public AllocationPool {
    public:
        BufferPool() = default;

        [[nodiscard]] static Result<BufferPool, PoolResult> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            HeapManager& heap_manager,
            VkDeviceSize size,
            VkBufferUsageFlags usage
        );

        void free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept;

        inline VkBuffer& handle() noexcept { return this->buffer; }

    protected:
        BufferPool(VkDeviceMemory memory, VkDeviceSize size)
            : AllocationPool(memory, size) {
        }

        ExternalHandle<VkBuffer, VK_NULL_HANDLE> buffer;
    };

    class DynamicBufferPool : public BufferPool {
    public:
        DynamicBufferPool() = default;

        [[nodiscard]] static Result<DynamicBufferPool, PoolResult> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            HeapManager& heap_manager,
            VkDeviceSize size,
            VkBufferUsageFlags usage
        );

        ~DynamicBufferPool();

        DynamicBufferPool(DynamicBufferPool&&) noexcept = default;

        DynamicBufferPool& operator=(DynamicBufferPool&&) = default;

        [[nodiscard]] PoolResult map(const VolkDeviceTable& fn_table, VkDevice device, u8 frame_mod);

        void unmap(const VolkDeviceTable& fn_table, VkDevice device);

        inline void** host_ptr() const noexcept { return this->mapped_host_ptr.get(); }

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
        DynamicBufferPool(VkDeviceMemory memory, VkDeviceSize size)
            : BufferPool(memory, size) {
        }

        // Use unique pointer so that the location never changes, even if the pool is moved. Resource handles will have
        // a pointer to this pointer.
        // Unique pointers cannot be copied, so there is no need to wrap this in an `Uncopyable`.
        std::unique_ptr<void*> mapped_host_ptr;
    };

    class TexturePool : public AllocationPool {
    public:
        TexturePool() = default;

        [[nodiscard]] static std::expected<TexturePool, PoolResult> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            HeapManager& heap_manager,
            VkDeviceSize size,
            u32 memory_type_bits
        );
        void free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept;

        std::expected<PoolRange, PoolResult> allocate(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            VkImage image,
            VkDeviceSize size,
            VkDeviceSize alignment
        );

    private:
        TexturePool(VkDeviceMemory memory, VkDeviceSize size)
            : AllocationPool(memory, size) {
        }
    };
}
