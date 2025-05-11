#include <pch.hpp>

#include "heap_manager.hpp"

#include <util/flow.hpp>

static constexpr VkMemoryPropertyFlags MAIN_REQUIRED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
static constexpr VkMemoryPropertyFlags MAIN_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

static constexpr VkMemoryPropertyFlags DYNAMIC_REQUIRED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
static constexpr VkMemoryPropertyFlags DYNAMIC_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

static constexpr VkMemoryPropertyFlags UPLOAD_REQUIRED_FLAGS = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
static constexpr VkMemoryPropertyFlags UPLOAD_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

static constexpr VkMemoryPropertyFlags DOWNLOAD_REQUIRED_FLAGS = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
static constexpr VkMemoryPropertyFlags DOWNLOAD_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

namespace hc::render::device::memory {
    Result<HeapManager, HeapResult> HeapManager::create(VkPhysicalDevice physical_device) {
        HeapManager manager;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &manager.mem_properties);

        constexpr u32 unassigned_idx = std::numeric_limits<u32>::max();
        for (auto& heap_index : manager.heap_indexes) {
            heap_index = unassigned_idx;
        }

        for (u32 i = 0; i < manager.mem_properties.memoryTypeCount; i++) {
            VkMemoryPropertyFlags flags = manager.mem_properties.memoryTypes[i].propertyFlags;

            HC_TRACE(
                "Memory type " << i << " properties: "
                //<< '(' << std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(flags) << ") => "
                << (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL | " : "") << (flags &
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE | " : "") << (flags &
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT | " : "") << (flags &
                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED | " : "") << (flags &
                    VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED | " : "") << (flags &
                    VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED | " : "") << (flags ? "\b\b  " : "NONE")
            );

            if (manager.heap_indexes[static_cast<Sz>(Heap::Main)] == unassigned_idx || manager.mem_properties.
                memoryTypes[manager.heap_indexes[static_cast<Sz>(Heap::Main)]].propertyFlags != MAIN_REQUIRED_FLAGS) {
                if (flags == MAIN_REQUIRED_FLAGS) {
                    manager.heap_indexes[static_cast<Sz>(Heap::Main)] = i;
                    continue;
                }

                if (manager.heap_indexes[static_cast<Sz>(Heap::Main)] == unassigned_idx && flags & MAIN_REQUIRED_FLAGS
                    && !(flags & MAIN_UNWANTED_FLAGS))
                    manager.heap_indexes[static_cast<Sz>(Heap::Main)] = i;
            }

            if (manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] == unassigned_idx || manager.mem_properties.
                memoryTypes[manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)]].propertyFlags != (
                    DYNAMIC_REQUIRED_FLAGS | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                if (flags == (DYNAMIC_REQUIRED_FLAGS | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] = i;
                    continue;
                }

                if (manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] == unassigned_idx && flags &
                    DYNAMIC_REQUIRED_FLAGS && !(flags & DYNAMIC_UNWANTED_FLAGS))
                    manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] = i;
            }

            if (manager.heap_indexes[static_cast<Sz>(Heap::Upload)] == unassigned_idx || manager.mem_properties.
                memoryTypes[manager.heap_indexes[static_cast<Sz>(Heap::Upload)]].propertyFlags !=
                UPLOAD_REQUIRED_FLAGS) {
                if (flags == UPLOAD_REQUIRED_FLAGS) {
                    manager.heap_indexes[static_cast<Sz>(Heap::Upload)] = i;
                    continue;
                }

                if (manager.heap_indexes[static_cast<Sz>(Heap::Upload)] == unassigned_idx && flags &
                    UPLOAD_REQUIRED_FLAGS && !(flags & UPLOAD_UNWANTED_FLAGS))
                    manager.heap_indexes[static_cast<Sz>(Heap::Upload)] = i;
            }

            if (manager.heap_indexes[static_cast<Sz>(Heap::Download)] == unassigned_idx || manager.mem_properties.
                memoryTypes[manager.heap_indexes[static_cast<Sz>(Heap::Download)]].propertyFlags !=
                DOWNLOAD_REQUIRED_FLAGS) {
                if (flags == DOWNLOAD_REQUIRED_FLAGS) {
                    manager.heap_indexes[static_cast<Sz>(Heap::Download)] = i;
                    continue;
                }

                if (manager.heap_indexes[static_cast<Sz>(Heap::Download)] == unassigned_idx && flags &
                    DOWNLOAD_REQUIRED_FLAGS && !(flags & DOWNLOAD_UNWANTED_FLAGS))
                    manager.heap_indexes[static_cast<Sz>(Heap::Download)] = i;
            }
        }

        // Check for heaps that haven't been assigned
        bool missing_heaps = false;
        if (manager.heap_indexes[static_cast<Sz>(Heap::Main)] == unassigned_idx) {
            HC_ERROR("Could not find suitable main heap");
            missing_heaps = true;
        }
        if (manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] == unassigned_idx) {
            HC_ERROR("Could not find suitable dynamic heap");
            missing_heaps = true;
        }
        if (manager.heap_indexes[static_cast<Sz>(Heap::Upload)] == unassigned_idx) {
            HC_ERROR("Could not find suitable upload heap");
            missing_heaps = true;
        }
        if (manager.heap_indexes[static_cast<Sz>(Heap::Download)] == unassigned_idx) {
            HC_ERROR("Could not find suitable download heap");
            missing_heaps = true;
        }
        if (missing_heaps)
            return Err(HeapResult::HeapNotFound);

        HC_TRACE(
            "Heap type indexes: " << "main = " << manager.heap_indexes[static_cast<Sz>(Heap::Main)] << " ; dynamic = "
            << manager.heap_indexes[static_cast<Sz>(Heap::Dynamic)] << " ; upload = " << manager.heap_indexes[
                static_cast<Sz>(Heap::Upload)] << " ; download = " << manager.heap_indexes[static_cast<Sz>(Heap::
                Download)]
        );

        if (manager.host_coherent_dynamic_heap()) {
            HC_DEBUG("Dynamic heap is host coherent");
        } else {
            HC_DEBUG("Dynamic heap is NOT host coherent");
        }

        if (manager.host_coherent_upload_heap()) {
            HC_DEBUG("Upload heap is host coherent");
        } else {
            HC_DEBUG("Upload heap is NOT host coherent");
        }

        return Ok(std::move(manager));
    }

    HeapResult HeapManager::alloc_buffer(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkDeviceMemory& memory,
        VkBuffer& buffer,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        Heap heap
    ) noexcept {
        HC_ASSERT(heap != Heap::MaxEnum, "Heap must be valid");

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult res = fn_table.vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
        switch (res) {
        case VK_SUCCESS:
            // Nothing, keep going
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return HeapResult::OutOfHostMemory;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return HeapResult::OutOfDeviceMemory;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
            return HeapResult::InvalidCapture;
        default: HC_UNREACHABLE("vkCreateBuffer should not return any other VkResult values");
        }

        VkMemoryRequirements memory_requirements;
        fn_table.vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
        if (!this->is_valid_heap(heap, memory_requirements.memoryTypeBits)) {
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return HeapResult::UnsupportedHeap;
        }

        u32 heap_index = this->heap_indexes[static_cast<Sz>(heap)];

        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.allocationSize = memory_requirements.size;
        memory_info.memoryTypeIndex = heap_index;

        auto memory_res = this->allocate_memory(fn_table, device, memory_info);
        if (!memory_res) {
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return memory_res.error();
        }
        memory = *memory_res;

        res = fn_table.vkBindBufferMemory(device, buffer, memory, 0);
        if (res != VK_SUCCESS) {
            this->free(fn_table, device, memory);
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        switch (res) {
        case VK_SUCCESS:
            // Nothing, keep going
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return HeapResult::OutOfHostMemory;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return HeapResult::OutOfDeviceMemory;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
            return HeapResult::InvalidCapture;
        default: HC_UNREACHABLE("vkBindBufferMemory should not return any other VkResult values");
        }

        return HeapResult::Success;
    }

    std::expected<void, HeapResult> HeapManager::alloc_texture_memory(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkDeviceMemory& memory,
        VkDeviceSize size,
        Heap heap,
        u32 memory_type_bits
    ) {
        HC_ASSERT(heap != Heap::MaxEnum, "Heap must be valid");

        if (!this->is_valid_heap(heap, memory_type_bits)) {
            return std::unexpected(HeapResult::UnsupportedHeap);
        }
        u32 heap_index = this->heap_indexes[static_cast<Sz>(heap)];

        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.allocationSize = size;
        memory_info.memoryTypeIndex = heap_index;

        auto res = this->allocate_memory(fn_table, device, memory_info);
        if (!res) {
            return std::unexpected(res.error());
        }
        memory = *res;

        return {};
    }

    void HeapManager::free(const VolkDeviceTable& fn_table, VkDevice device, VkDeviceMemory& memory) noexcept {
        // Having to call free from the heap manager instead of freeing the memory directly is a bit silly, but since
        // the number of allocations will be counted for debugging and profiling, may as well do it like this
        // it also falls more inline with the purpose of the heap manager
        fn_table.vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        this->allocation_count--;
    }

    // TODO is this function even needed?
    u32 HeapManager::find_memory_type(u32 type_filter, VkMemoryPropertyFlags heap_properties) {
        // Exact type search
        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1U << i)) && this->mem_properties.memoryTypes[i].propertyFlags == heap_properties) {
                return i;
            }
        }
        HC_WARN("Failed to find exact memory type.");

        // Relaxed search
        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1U << i)) && (this->mem_properties.memoryTypes[i].propertyFlags & heap_properties) ==
                heap_properties) {
                return i;
            }
        }

        // TODO not sure how to deal with this
        HC_ERROR(
            "Failed to find suitable memory type. Type bit mask: " << std::bitset<sizeof(type_filter) * 8>(type_filter)
            << " Property flags: " << std::bitset<sizeof(heap_properties) * 8>(heap_properties)
        );
        HC_UNREACHABLE("A suitable memory type must be found");
    }

    bool HeapManager::is_valid_heap(Heap heap, u32 memory_type_bits) const noexcept {
        HC_ASSERT(heap != Heap::MaxEnum, "Heap must be valid");
        return 1U << this->heap_indexes[static_cast<Sz>(heap)] & memory_type_bits;
    }

    std::vector<u32> HeapManager::valid_heaps(u32 memory_type_bits) const noexcept {
        std::vector<u32> res;
        res.reserve(std::popcount(memory_type_bits));

        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            if (1U << i & memory_type_bits) {
                res.push_back(i);
            }
        }

        return res;
    }

    std::expected<VkDeviceMemory, HeapResult> HeapManager::allocate_memory(VolkDeviceTable const& fn_table, VkDevice device, VkMemoryAllocateInfo const& memory_info) {
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkResult res = fn_table.vkAllocateMemory(device, &memory_info, nullptr, &memory);

        switch (res) {
        case VK_SUCCESS:
            // Nothing, keep going
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return std::unexpected(HeapResult::OutOfHostMemory);
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return std::unexpected(HeapResult::OutOfDeviceMemory);
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
            return std::unexpected(HeapResult::InvalidCapture);
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return std::unexpected(HeapResult::InvalidHandle);
        default: HC_UNREACHABLE("vkAllocateMemory should not return any other VkResult values");
        }

        this->allocation_count++;

        return memory;
    }
}
