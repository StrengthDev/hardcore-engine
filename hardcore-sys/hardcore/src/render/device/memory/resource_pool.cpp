#include <pch.hpp>

#include "resource_pool.hpp"

#include <render/vars.hpp>

namespace hc::render::device::memory {
    Result<BufferPool, PoolResult> BufferPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        VkBufferUsageFlags usage
    ) {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;

        HeapResult res = heap_manager.alloc_buffer(fn_table, device, memory, buffer, size, usage, Heap::Main);

        switch (res) {
        case HeapResult::Success:
            // Nothing, keep going
            break;
        case HeapResult::OutOfHostMemory:
            return Err(PoolResult::OutOfHostMemory);
        case HeapResult::OutOfDeviceMemory:
            return Err(PoolResult::OutOfDeviceMemory);
        case HeapResult::UnsupportedHeap:
            return Err(PoolResult::UnsupportedHeap);
        // These are both ignored because a specific address is never requested, and no external handle is used
        // case HeapResult::InvalidCapture:
        // case HeapResult::InvalidHandle:
        default: HC_UNREACHABLE("alloc_buffer should not return any other values here");
        }

        BufferPool pool = BufferPool(memory, size);
        pool.buffer = buffer;

        return Ok(std::move(pool));
    }

    void BufferPool::free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            fn_table.vkDestroyBuffer(device, this->buffer, nullptr);
            this->buffer.destroy();
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    Result<DynamicBufferPool, PoolResult> DynamicBufferPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        VkBufferUsageFlags usage
    ) {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;

        u8 max_frames_in_flight = render::max_frames_in_flight();
        HeapResult res = heap_manager.alloc_buffer(
            fn_table,
            device,
            memory,
            buffer,
            size * max_frames_in_flight,
            usage,
            Heap::Dynamic
        );

        switch (res) {
        case HeapResult::Success:
            // Nothing, keep going
            break;
        case HeapResult::OutOfHostMemory:
            return Err(PoolResult::OutOfHostMemory);
        case HeapResult::OutOfDeviceMemory:
            return Err(PoolResult::OutOfDeviceMemory);
        case HeapResult::UnsupportedHeap:
            return Err(PoolResult::UnsupportedHeap);
        // These are both ignored because a specific address is never requested, and no external handle is used
        // case HeapResult::InvalidCapture:
        // case HeapResult::InvalidHandle:
        default: HC_UNREACHABLE("alloc_buffer should not return any other values here");
        }

        DynamicBufferPool pool = DynamicBufferPool(memory, size);
        pool.buffer = buffer;
        pool.mapped_host_ptr = std::make_unique<void*>(nullptr);

        return Ok(std::move(pool));
    }

    DynamicBufferPool::~DynamicBufferPool() {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory not unmapped");
    }

    PoolResult DynamicBufferPool::map(const VolkDeviceTable& fn_table, VkDevice device, u8 frame_mod) {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory already mapped");
        VkResult res = fn_table.vkMapMemory(
            device,
            this->memory,
            this->total_capacity * frame_mod,
            this->total_capacity,
            0,
            this->mapped_host_ptr.get()
        );

        switch (res) {
        case VK_SUCCESS:
            return PoolResult::Success;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return PoolResult::OutOfHostMemory;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return PoolResult::OutOfDeviceMemory;
        case VK_ERROR_MEMORY_MAP_FAILED:
            return PoolResult::MapFailure;
        default: HC_UNREACHABLE("vkMapMemory should not return any other VkResult values");
        }
    }

    void DynamicBufferPool::unmap(const VolkDeviceTable& fn_table, VkDevice device) {
        HC_ASSERT(this->mapped_host_ptr != nullptr, "Memory not mapped");
        fn_table.vkUnmapMemory(device, this->memory);
        this->mapped_host_ptr = nullptr;
    }

    std::expected<TexturePool, PoolResult> TexturePool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager,
        VkDeviceSize size,
        u32 memory_type_bits
    ) {
        VkDeviceMemory memory = VK_NULL_HANDLE;

        auto res = heap_manager.alloc_texture_memory(fn_table, device, memory, size, Heap::Main, memory_type_bits);
        if (!res) {
            switch (res.error()) {
            case HeapResult::Success:
                // Nothing, keep going
                break;
            case HeapResult::OutOfHostMemory:
                return std::unexpected(PoolResult::OutOfHostMemory);
            case HeapResult::OutOfDeviceMemory:
                return std::unexpected(PoolResult::OutOfDeviceMemory);
            case HeapResult::UnsupportedHeap:
                return std::unexpected(PoolResult::UnsupportedHeap);
            // These are both ignored because a specific address is never requested, and no external handle is used
            // case HeapResult::InvalidCapture:
            // case HeapResult::InvalidHandle:
            default: HC_UNREACHABLE("alloc_texture_memory should not return any other values here");
            }
        }

        return TexturePool(memory, size);
    }

    void TexturePool::free(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    std::expected<PoolRange, PoolResult> TexturePool::allocate(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkImage image,
        VkDeviceSize size,
        VkDeviceSize alignment
    ) {
        auto range_res = AllocationPool::allocate(size, alignment);
        if (!range_res) {
            return std::unexpected(PoolResult::NotEnoughSpace);
        }
        PoolRange range = *range_res;

        VkResult res = fn_table.vkBindImageMemory(device, image, this->memory, range.offset + range.padding);
        if (res != VK_SUCCESS) {
            this->free_allocation(range.offset);
            switch (res) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return std::unexpected(PoolResult::OutOfHostMemory);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return std::unexpected(PoolResult::OutOfDeviceMemory);
            default: HC_UNREACHABLE("vkBindImageMemory should not return any other values here");
            }
        }

        return range;
    }
}
