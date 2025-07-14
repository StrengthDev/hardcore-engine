#include <pch.hpp>

#include "resource_pool.hpp"

#include <render/util.hpp>
#include <render/vars.hpp>

namespace hc::render::device::memory {
    std::expected<BufferPool, Error> BufferPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        VkBufferUsageFlags usage
    ) {
        auto result = heap_manager.alloc_buffer(fn_table, device, size, usage, Heap::Main);
        if (!result) {
            return result.error();
        }
        auto [memory, buffer] = *result;

        BufferPool pool = BufferPool(memory, size);
        pool.buffer = buffer;

        return std::move(pool);
    }

    void BufferPool::free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            fn_table.vkDestroyBuffer(device, this->buffer, nullptr);
            this->buffer.destroy();
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    std::expected<DynamicBufferPool, Error> DynamicBufferPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        VkBufferUsageFlags usage
    ) {
        u8 max_frames_in_flight = render::max_frames_in_flight();
        auto result = heap_manager.alloc_buffer(
            fn_table,
            device,
            size * max_frames_in_flight,
            usage,
            Heap::Dynamic
        );
        if (!result) {
            return result.error();
        }
        auto [memory, buffer] = *result;

        DynamicBufferPool pool = DynamicBufferPool(memory, size);
        pool.buffer = buffer;
        pool.mapped_host_ptr = std::make_unique<void*>(nullptr);

        return std::move(pool);
    }

    DynamicBufferPool::~DynamicBufferPool() {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory not unmapped");
    }

    std::expected<void, Error> DynamicBufferPool::map(const VolkDeviceTable& fn_table, VkDevice device, u8 frame_mod) {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory already mapped");

        VkResult result = fn_table.vkMapMemory(
            device,
            this->memory,
            this->total_capacity * frame_mod,
            this->total_capacity,
            0,
            this->mapped_host_ptr.get()
        );

        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to map buffer: " << to_str(result));
            return Error(result);
        }

        return {};
    }

    void DynamicBufferPool::unmap(const VolkDeviceTable& fn_table, VkDevice device) {
        HC_ASSERT(this->mapped_host_ptr != nullptr, "Memory not mapped");
        fn_table.vkUnmapMemory(device, this->memory);
        this->mapped_host_ptr = nullptr;
    }

    std::expected<TexturePool, Error> TexturePool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        u32 memory_type_bits
    ) {
        auto result = heap_manager.alloc_texture_memory(fn_table, device, size, Heap::Main, memory_type_bits);
        if (!result) {
            return result.error();
        }

        return TexturePool(*result, size);
    }

    void TexturePool::free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept {
        if (this->memory.valid()) {
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    std::expected<PoolRange, Error> TexturePool::allocate(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkImage image,
        VkDeviceSize size,
        VkDeviceSize alignment
    ) {
        auto range_res = AllocationPool::allocate(size, alignment);
        if (!range_res) {
            return Error(HCError_CouldNotFitInPool);
        }
        PoolRange range = *range_res;

        VkResult result = fn_table.vkBindImageMemory(device, image, this->memory, range.offset + range.padding);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to bind texture memory: " << to_str(result));
            this->free_allocation(range.offset);
            return Error(result);
        }

        return range;
    }
}
