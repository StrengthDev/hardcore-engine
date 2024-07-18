#include <pch.hpp>

#include <render/vars.hpp>

#include "resource_pool.hpp"

namespace hc::device::memory {
    const u32 INITIAL_POOL_SLOT_SIZE = 16;

    resource_pool::resource_pool(VkDeviceSize size, bool per_frame_allocation) :
            capacity(size), slots(INITIAL_POOL_SLOT_SIZE) {
        this->slots[0].in_use = false;
        this->slots[0].offset = 0;
        this->slots[0].size = size;
        this->largest_free_slot = 0;

        for (u32 i = 1; i < this->slots.size(); i++) {
            this->slots[i].in_use = false;
            this->slots[i].offset = size;
            this->slots[i].size = 0;
        }

        this->per_frame_allocation = per_frame_allocation;
    }

    resource_pool::~resource_pool() {
        HC_ASSERT(this->memory == VK_NULL_HANDLE, "Resource pool not freed");
    }

    void
    resource_pool::free_memory(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            heap_manager.free(fn_table, device, this->memory);
            this->slots.clear();
        }
    }

    // TODO could try making allocations more efficient my reducing the size of the alignment padding
    bool resource_pool::search(VkDeviceSize size, VkDeviceSize alignment,
                               u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const {
        if (this->capacity < size)
            return false;

        constexpr u32 invalid_idx = std::numeric_limits<u32>::max();

        if (this->slots[this->largest_free_slot].in_use) {
            // Largest free slot is unknown, must check everything, look for smallest possible fit
            // if possible, update the largest free slot to reduce cost of future calls

            VkDeviceSize largest_fit = 0;
            u32 largest_idx = invalid_idx;
            VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
            u32 smallest_idx = invalid_idx;
            VkDeviceSize smallest_offset = 0;

            VkDeviceSize offset = 0;
            for (u32 i = 0; i < this->slots.size(); i++) {
                offset += this->slots[i].offset;

                if (this->slots[i].in_use)
                    continue;

                if (largest_fit < this->slots[i].size) {
                    largest_fit = this->slots[i].size;
                    largest_idx = i;
                }

                if (smallest_fit > this->slots[i].size &&
                    size + alignment_pad(this->slots[i].offset, alignment) <= this->slots[i].size) {
                    smallest_fit = this->slots[i].size;
                    smallest_idx = i;
                    smallest_offset = offset;
                }
            }

            if (smallest_idx != invalid_idx) {
                if (largest_idx != invalid_idx && largest_idx != smallest_idx)
                    this->largest_free_slot = largest_idx;

                out_slot_idx = smallest_idx;
                out_size_needed = size + alignment_pad(this->slots[smallest_idx].offset, alignment);
                out_offset = smallest_offset;
                return true;
            } else {
                if (largest_idx != invalid_idx)
                    this->largest_free_slot = largest_idx;

                return false;
            }
        } else {
            // Largest free slot is known, so if it is large enough, assign a slot in this pool,
            // otherwise move on to next pool

            if (this->slots[this->largest_free_slot].size < size)
                return false;

            VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
            u32 smallest_idx = invalid_idx;
            VkDeviceSize smallest_offset = 0;

            VkDeviceSize offset = 0;
            for (u32 i = 0; i < this->slots.size(); i++) {
                offset += this->slots[i].offset;

                if (!this->slots[i].in_use && smallest_fit > this->slots[i].size &&
                    size + alignment_pad(this->slots[i].offset, alignment) <= this->slots[i].size) {
                    smallest_fit = this->slots[i].size;
                    smallest_idx = i;
                    smallest_offset = offset;
                }
            }

            out_slot_idx = smallest_idx;
            out_size_needed = size + alignment_pad(this->slots[smallest_idx].offset, alignment);
            out_offset = smallest_offset;
            return true;
        }
    }

    void resource_pool::fill_slot(u32 slot_idx, VkDeviceSize size) {
        HC_ASSERT(slot_idx < this->slots.size(), "Slot index out of bounds");
        HC_ASSERT(size <= this->slots[slot_idx].size, "Allocation size greater than slot size");
        HC_ASSERT((slot_idx == 0) | this->slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use.");

        if (size < this->slots[slot_idx].size) {
            const u32 old_size = this->slots.size();
            bool selected_last = slot_idx == (this->slots.size() - 1);

            // Resize needed
            if (selected_last ||
                (this->slots[slot_idx + 1].in_use && this->slots[this->slots.size() - 1].in_use)) {
                push_back_empty();
                this->slots[old_size].in_use = false;
                this->slots[old_size].offset = this->slots[old_size - 1].offset + this->slots[old_size - 1].size;
                this->slots[old_size].size = 0;
            }

            HC_ASSERT(this->slots[slot_idx + 1].in_use || !this->slots[slot_idx + 1].size,
                      "Slot after the selected slot must be either empty or in use");

            // Shift remaining slots one position and add new slot with the remaining unused memory
            if (this->slots[slot_idx + 1].in_use) {
                u32 last_in_use = 0;
                for (u32 i = old_size - 1; i > slot_idx; i--) {
                    if (this->slots[i].in_use) {
                        last_in_use = i;
                        break;
                    }
                }
                move_slots(slot_idx + 2, slot_idx + 1, last_in_use - slot_idx);
            }

            this->slots[slot_idx + 1].in_use = false;
            this->slots[slot_idx + 1].offset = this->slots[slot_idx].offset + size;
            this->slots[slot_idx + 1].size = this->slots[slot_idx].size - size;
        }

        this->slots[slot_idx].in_use = true;
        this->slots[slot_idx].size = size;
    }

    u32 resource_pool::find_slot(VkDeviceSize offset) const noexcept {
        VkDeviceSize total_offset = 0;
        u32 i = 0;
        for (; i < this->slots.size() && total_offset < offset; i++)
            total_offset += this->slots[i].offset;

        if (total_offset == offset)
            return i;
        else
            return std::numeric_limits<u32>::max();
    }

    buffer_pool::buffer_pool(VkDevice device, HeapManager &heap_manager,
                             VkDeviceSize size, VkBufferUsageFlags usage, Heap heap,
                             bool per_frame_allocation) :
            resource_pool(size, per_frame_allocation) {
        u8 max_frames_in_flight = hc::render::max_frames_in_flight();
        heap_manager.alloc_buffer(device, this->memory, this->buffer,
                                  per_frame_allocation ? size * max_frames_in_flight : size,
                                  usage, heap);
    }

    void buffer_pool::free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            fn_table.vkDestroyBuffer(device, this->buffer, nullptr);
            this->free_memory(fn_table, device, heap_manager);
        }
    }

    void buffer_pool::push_back_empty() {
        this->slots.push_back({});
    }

    void buffer_pool::move_slots(u32 dst, u32 src, u32 count) {
        std::memmove(&this->slots.data()[dst], &this->slots.data()[src], sizeof(*this->slots.data()) * count);
    }

    void dynamic_buffer_pool::map(VkDevice device, u8 current_frame) {
        VK_CRASH_CHECK(
                vkMapMemory(device, this->memory, this->capacity * current_frame, this->capacity, 0,
                            &this->mapped_host_ptr),
                "Failed to map memory");
    }

    void dynamic_buffer_pool::unmap(VkDevice device) {
        HC_ASSERT(this->mapped_host_ptr != nullptr, "Memmory not mapped");
        vkUnmapMemory(device, this->memory);
        this->mapped_host_ptr = nullptr;
    }

    void dynamic_buffer_pool::push_flush_ranges(std::vector<VkMappedMemoryRange> &ranges, u8 current_frame,
                                                const std::vector<dynamic_buffer_pool> &pools) {
        for (const dynamic_buffer_pool &pool: pools) {
            // TODO skip pools with no allocations, need to track allocations somehow

            ranges.push_back(pool.mapped_range(current_frame));
        }
    }

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
            resource_pool(size), m_texture_slots(INITIAL_POOL_SLOT_SIZE) {
        this->m_memory_type_idx = heap_manager.alloc_texture_memory(device, this->memory, size, preferred_heap,
                                                                    memory_type_bits);
    }

    void texture_pool::free(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
        this->free_memory(fn_table, device, heap_manager);
        this->m_texture_slots.clear();
    }

    bool texture_pool::search(VkDeviceSize size, VkDeviceSize alignment, u32 memory_type_bits,
                              u32 &out_slot_idx, VkDeviceSize &out_size_needed, VkDeviceSize &out_offset) const {
        if (memory_type_bits & (1ULL << this->m_memory_type_idx))
            return resource_pool::search(size, alignment, out_slot_idx, out_size_needed, out_offset);

        return false;
    }

    void texture_pool::fill_slot(VkDevice device, texture_slot &&tex, u32 slot_idx, VkDeviceSize size,
                                 VkDeviceSize alignment) {
        resource_pool::fill_slot(slot_idx, size);

        // resource_pool::fill_slot should perform any reallocation if necessary, so the slot should always exist

        this->m_texture_slots[slot_idx] = std::move(tex);

        VK_CRASH_CHECK(vkBindImageMemory(device, this->m_texture_slots[slot_idx].image, this->memory,
                                         aligned_offset(this->slots[slot_idx].offset, alignment)),
                       "Failed to bind image to memory");
    }

    void texture_pool::push_back_empty() {
        this->slots.push_back({});
        this->m_texture_slots.push_back({});
    }

    void texture_pool::move_slots(u32 dst, u32 src, u32 count) {
        std::memmove(&this->slots.data()[dst], &this->slots.data()[src], sizeof(*this->slots.data()) * count);
        std::memmove(&this->m_texture_slots.data()[dst], &this->m_texture_slots.data()[src],
                     sizeof(*this->m_texture_slots.data()) * count);
    }

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
