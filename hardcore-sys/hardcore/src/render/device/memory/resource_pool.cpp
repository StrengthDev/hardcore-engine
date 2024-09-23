#include <pch.hpp>

#include <render/vars.hpp>

#include "resource_pool.hpp"

namespace hc::render::device::memory {
    Result<BufferPool, PoolResult> BufferPool::create(const VolkDeviceTable &fn_table, VkDevice device,
                                                      hc::render::device::memory::HeapManager &heap_manager,
                                                      VkDeviceSize size, VkBufferUsageFlags usage) {
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

        BufferPool pool = BufferPool(memory, size, false);
        pool.buffer = buffer;

        return Ok(std::move(pool));
    }

    void BufferPool::free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            fn_table.vkDestroyBuffer(device, this->buffer, nullptr);
            this->buffer.destroy();
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    Result<DynamicBufferPool, PoolResult> DynamicBufferPool::create(const VolkDeviceTable &fn_table,
                                                                    VkDevice device,
                                                                    hc::render::device::memory::HeapManager &heap_manager,
                                                                    VkDeviceSize size, VkBufferUsageFlags usage) {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;

        u8 max_frames_in_flight = hc::render::max_frames_in_flight();
        HeapResult res = heap_manager.alloc_buffer(fn_table, device, memory, buffer, size * max_frames_in_flight,
                                                   usage, Heap::Dynamic);

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
        pool.mapped_host_ptr = std::make_unique<void *>(nullptr);

        return Ok(std::move(pool));
    }

    DynamicBufferPool::~DynamicBufferPool() {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory not unmapped");
    }

    PoolResult DynamicBufferPool::map(const VolkDeviceTable &fn_table, VkDevice device, u8 frame_mod) {
        HC_ASSERT(this->mapped_host_ptr == nullptr, "Memory already mapped");
        VkResult res = fn_table.vkMapMemory(device, this->memory, this->total_capacity * frame_mod,
                                            this->total_capacity, 0,
                                            this->mapped_host_ptr.get());

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

    void DynamicBufferPool::unmap(const VolkDeviceTable &fn_table, VkDevice device) {
        HC_ASSERT(this->mapped_host_ptr != nullptr, "Memory not mapped");
        fn_table.vkUnmapMemory(device, this->memory);
        this->mapped_host_ptr = nullptr;
    }
/*
    texture_slot
    create_texture(VkDevice device, VkImageCreateInfo image_info, VkMemoryRequirements &out_memory_requirements) {
        VkImage image;
        VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &image), "Failed to create image");

        vkGetImageMemoryRequirements(device, image, &out_memory_requirements);

        VkImageView image_view;

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.flags = 0;
        view_info.image = image;
        view_info.format = image_info.format;

        // Setting all components to identity so no swizzling occurs
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = image_info.mipLevels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = image_info.arrayLayers;

        const bool is_cube = image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        HC_ASSERT(!is_cube || (is_cube && image_info.imageType == VK_IMAGE_TYPE_2D),
                  "Image type must be 2D if image is a cube");
        HC_ASSERT(!is_cube || (is_cube && image_info.arrayLayers % 6 == 0),
                  "Number of image layers must be a multiple of 6 if image is a cube");

        const bool is_array = is_cube ? 1 < image_info.arrayLayers / 6 : 1 < image_info.arrayLayers;
        HC_ASSERT(!is_array || (is_array && image_info.imageType != VK_IMAGE_TYPE_3D),
                  "3D image arrays are invalid");

        switch (image_info.imageType) {
            case VK_IMAGE_TYPE_1D:
                view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
                break;
            case VK_IMAGE_TYPE_2D:
                if (is_cube) view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
                else view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                break;
            case VK_IMAGE_TYPE_3D:
                view_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
            default:
            HC_ASSERT(false, "Invalid image type");
                break;
        }

        //VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &image_view), "Failed to create image view");

        return {
                .image = image,
                .view = VK_NULL_HANDLE,// image_view,
                .size = out_memory_requirements.size,
                .dims = image_info.extent,
                .layout = image_info.initialLayout
        };
    }

    texture_pool::texture_pool(VkDevice device, HeapManager &heap_manager, VkDeviceSize size,
                               u32 memory_type_bits, Heap preferred_heap) :
            AllocationPool<TextureSlot>(size) {
        this->m_memory_type_idx = heap_manager.alloc_texture_memory(device, this->memory, size, preferred_heap,
                                                                    memory_type_bits);
    }

    void texture_pool::free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
        this->free_memory(fn_table, device, heap_manager);
        this->extra_slots.clear();
    }

    bool texture_pool::search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
                              u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const {
        if (memory_type_bits & (1ULL << this->m_memory_type_idx))
            return AllocationPool<TextureSlot>::search(size, alignment, out_slot_idx, out_size_needed, out_offset);

        return false;
    }

    void texture_pool::fill_slot(VkDevice device, TextureSlot &&tex, u32 slot_idx, VkDeviceSize size,
                                 VkDeviceSize alignment) {
        AllocationPool<TextureSlot>::fill_slot(slot_idx, size);

        // resource_pool::fill_slot should perform any reallocation if necessary, so the slot should always exist

        this->extra_slots[slot_idx] = std::move(tex);

        VK_CRASH_CHECK(vkBindImageMemory(device, this->extra_slots[slot_idx].image, this->memory,
                                         aligned_offset(this->slots[slot_idx].offset, alignment)),
                       "Failed to bind image to memory");
    }
*/
#ifdef TODO

    struct dynamic_texture {
        VkImage image;
        VkImageLayout layout;
        std::array<VkImageView, max_frames_in_flight> views;
    };

    dynamic_texture create_dynamic_texture(VkDevice device, VkImageCreateInfo image_info,
                                           VkMemoryRequirements &out_memory_requirements) {
        dynamic_texture res = {};
        u8 max_frames_in_flight = hc::render::max_frames_in_flight();

        VK_CRASH_CHECK(vkCreateImage(device, &image_info, nullptr, &res.image), "Failed to create image");

        vkGetImageMemoryRequirements(device, res.image, &out_memory_requirements);

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.flags = 0;
        view_info.image = res.image;
        view_info.format = image_info.format;

        // Setting all components to identity so no swizzling occurs
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        HC_ASSERT(image_info.arrayLayers % max_frames_in_flight == 0,
                        "Number of layers indivisible by number of frames in flight");
        const u32 frame_layers = image_info.arrayLayers / max_frames_in_flight;

        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = image_info.mipLevels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = frame_layers;

        const bool is_cube = image_info.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        HC_ASSERT(!is_cube || image_info.imageType == VK_IMAGE_TYPE_2D,
                        "Image type must be 2D if image is a cube");
        HC_ASSERT(!is_cube || frame_layers % 6 == 0,
                        "Number of image layers must be a multiple of 6 if image is a cube");

        const bool is_array = is_cube ? 1 < frame_layers / 6 : 1 < frame_layers;

        switch (image_info.imageType) {
            case VK_IMAGE_TYPE_1D:
                view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
                break;
            case VK_IMAGE_TYPE_2D:
                if (is_cube) view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
                else view_info.viewType = is_array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                break;
            default:
                HC_ASSERT(false, "Invalid image type");
                break;
        }

        for (u8 i = 0; i < max_frames_in_flight; i++)
            VK_CRASH_CHECK(vkCreateImageView(device, &view_info, nullptr, &res.views[i]),
                           "Failed to create image view");

        return res;
    }

#endif // TODO

}
