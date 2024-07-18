#include <pch.hpp>

#include "heap_manager.hpp"

const VkMemoryPropertyFlags MAIN_REQUIRED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
const VkMemoryPropertyFlags MAIN_UNWANTED_FLAGS =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

const VkMemoryPropertyFlags DYNAMIC_REQUIRED_FLAGS =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
const VkMemoryPropertyFlags DYNAMIC_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

const VkMemoryPropertyFlags UPLOAD_REQUIRED_FLAGS =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
const VkMemoryPropertyFlags UPLOAD_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

const VkMemoryPropertyFlags DOWNLOAD_REQUIRED_FLAGS =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
const VkMemoryPropertyFlags DOWNLOAD_UNWANTED_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

namespace hc::device::memory {
    HeapManager::HeapManager(VkPhysicalDevice physical_device) {
        vkGetPhysicalDeviceMemoryProperties(physical_device, &this->mem_properties);

        constexpr u32 unassigned_idx = std::numeric_limits<u32>::max();
        this->heap_indexes[static_cast<Sz>(Heap::Main)] = unassigned_idx;
        this->heap_indexes[static_cast<Sz>(Heap::Dynamic)] = unassigned_idx;
        this->heap_indexes[static_cast<Sz>(Heap::Upload)] = unassigned_idx;
        this->heap_indexes[static_cast<Sz>(Heap::Download)] = unassigned_idx;

        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            VkMemoryPropertyFlags flags = this->mem_properties.memoryTypes[i].propertyFlags;

            HC_INFO("Memory type " << i << " properties: "
                                   //<< '(' << std::bitset<sizeof(VkMemoryPropertyFlags) * 8>(flags) << ") => "
                                   << (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "DEVICE_LOCAL | " : "")
                                   << (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "HOST_VISIBLE | " : "")
                                   << (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "HOST_COHERENT | " : "")
                                   << (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "HOST_CACHED | " : "")
                                   << (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "LAZILY_ALLOCATED | " : "")
                                   << (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "PROTECTED | " : "")
                                   << (flags ? "\b\b  " : "NONE"));

            if (this->heap_indexes[static_cast<Sz>(Heap::Main)] == unassigned_idx ||
                this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Main)]].propertyFlags !=
                MAIN_REQUIRED_FLAGS) {
                if (flags == MAIN_REQUIRED_FLAGS) {
                    this->heap_indexes[static_cast<Sz>(Heap::Main)] = i;
                    continue;
                }

                if (this->heap_indexes[static_cast<Sz>(Heap::Main)] == unassigned_idx &&
                    flags & MAIN_REQUIRED_FLAGS &&
                    !(flags & MAIN_UNWANTED_FLAGS))
                    this->heap_indexes[static_cast<Sz>(Heap::Main)] = i;
            }

            if (this->heap_indexes[static_cast<Sz>(Heap::Dynamic)] == unassigned_idx ||
                this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Dynamic)]].propertyFlags !=
                (DYNAMIC_REQUIRED_FLAGS | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                if (flags == (DYNAMIC_REQUIRED_FLAGS | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    this->heap_indexes[static_cast<Sz>(Heap::Dynamic)] = i;
                    continue;
                }

                if (this->heap_indexes[static_cast<Sz>(Heap::Dynamic)] == unassigned_idx &&
                    flags & DYNAMIC_REQUIRED_FLAGS &&
                    !(flags & DYNAMIC_UNWANTED_FLAGS))
                    this->heap_indexes[static_cast<Sz>(Heap::Dynamic)] = i;
            }

            if (this->heap_indexes[static_cast<Sz>(Heap::Upload)] == unassigned_idx ||
                this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Upload)]].propertyFlags !=
                UPLOAD_REQUIRED_FLAGS) {
                if (flags == UPLOAD_REQUIRED_FLAGS) {
                    this->heap_indexes[static_cast<Sz>(Heap::Upload)] = i;
                    continue;
                }

                if (this->heap_indexes[static_cast<Sz>(Heap::Upload)] == unassigned_idx &&
                    flags & UPLOAD_REQUIRED_FLAGS &&
                    !(flags & UPLOAD_UNWANTED_FLAGS))
                    this->heap_indexes[static_cast<Sz>(Heap::Upload)] = i;
            }

            if (this->heap_indexes[static_cast<Sz>(Heap::Download)] == unassigned_idx ||
                this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Download)]].propertyFlags !=
                DOWNLOAD_REQUIRED_FLAGS) {
                if (flags == DOWNLOAD_REQUIRED_FLAGS) {
                    this->heap_indexes[static_cast<Sz>(Heap::Download)] = i;
                    continue;
                }

                if (this->heap_indexes[static_cast<Sz>(Heap::Download)] == unassigned_idx &&
                    flags & DOWNLOAD_REQUIRED_FLAGS &&
                    !(flags & DOWNLOAD_UNWANTED_FLAGS))
                    this->heap_indexes[static_cast<Sz>(Heap::Download)] = i;
            }
        }

        HC_INFO("Heap type indexes: "
                        << "main = " << this->heap_indexes[static_cast<Sz>(Heap::Main)]
                        << " ; dynamic = " << this->heap_indexes[static_cast<Sz>(Heap::Dynamic)]
                        << " ; upload = " << this->heap_indexes[static_cast<Sz>(Heap::Upload)]
                        << " ; download = " << this->heap_indexes[static_cast<Sz>(Heap::Download)]);

        if (this->host_coherent_dynamic_heap()) {
            HC_INFO("Dynamic heap is host coherent");
        } else {
            HC_INFO("Dynamic heap is NOT host coherent");
        }

        if (this->host_coherent_upload_heap()) {
            HC_INFO("Upload heap is host coherent");
        } else {
            HC_INFO("Upload heap is NOT host coherent");
        }
    }

    u32 HeapManager::find_memory_type(u32 type_filter, VkMemoryPropertyFlags heap_properties) {
        // Exact type search
        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && this->mem_properties.memoryTypes[i].propertyFlags == heap_properties) {
                return i;
            }
        }
        HC_WARN("Failed to find exact memory type.");

        // Relaxed search
        for (u32 i = 0; i < this->mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) &&
                (this->mem_properties.memoryTypes[i].propertyFlags & heap_properties) == heap_properties) {
                return i;
            }
        }

        // TODO not sure how to deal with this
        HC_ERROR("Failed to find suitable memory type. Type bit mask: "
                         << std::bitset<sizeof(type_filter) * 8>(type_filter) << " Property flags: "
                         << std::bitset<sizeof(heap_properties) * 8>(heap_properties));
        HC_UNREACHABLE("A suitable memory type must be found");
    }

    u32 HeapManager::get_memory_type_idx(Heap heap, u32 memory_type_bits) const {
        u32 res;

        switch (heap) {
            case Heap::Main:
                res = this->heap_indexes[static_cast<Sz>(Heap::Main)];
                break;
            case Heap::Dynamic:
                res = this->heap_indexes[static_cast<Sz>(Heap::Dynamic)];
                break;
            case Heap::Upload:
                res = this->heap_indexes[static_cast<Sz>(Heap::Upload)];
                break;
            case Heap::Download:
                res = this->heap_indexes[static_cast<Sz>(Heap::Download)];
                break;
            default: HC_UNREACHABLE("Heap must be valid");
        }
        // TODO this really shouldnt be just an assert, im assuming the chosen heap always meets the requirements
        HC_ASSERT(memory_type_bits & 1U << res, "Memory requirements must be met");

        return res;
    }

    HeapResult HeapManager::alloc_buffer(const VolkDeviceTable &fn_table, VkDevice device,
                                         VkDeviceMemory &memory, VkBuffer &buffer,
                                         VkDeviceSize size, VkBufferUsageFlags usage, Heap heap) noexcept {
        HC_ASSERT(heap != Heap::MaxEnum, "Heap must be valid");

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult res = fn_table.vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
        switch (res) {
            case VK_SUCCESS:
                // nothing, keep going
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return HeapResult::OutOfHostMemory;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return HeapResult::OutOfDeviceMemory;
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
                return HeapResult::InvalidCapture;
            default: HC_UNREACHABLE("vkCreateBuffer should not return any other VkResult values");
        }

        u32 heap_index = this->heap_indexes[static_cast<Sz>(heap)];

        VkMemoryRequirements memory_requirements;
        fn_table.vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
        if (!(memory_requirements.memoryTypeBits & 1U << heap_index)) {
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return HeapResult::UnsupportedHeap;
        }

        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.allocationSize = memory_requirements.size;
        memory_info.memoryTypeIndex = heap_index;

        res = fn_table.vkAllocateMemory(device, &memory_info, nullptr, &memory);
        if (res != VK_SUCCESS) {
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        switch (res) {
            case VK_SUCCESS:
                // nothing, keep going
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return HeapResult::OutOfHostMemory;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return HeapResult::OutOfDeviceMemory;
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
                return HeapResult::InvalidCapture;
            case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return HeapResult::InvalidHandle;
            default: HC_UNREACHABLE("vkAllocateMemory should not return any other VkResult values");
        }

        res = fn_table.vkBindBufferMemory(device, buffer, memory, 0);
        if (res != VK_SUCCESS) {
            fn_table.vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        switch (res) {
            case VK_SUCCESS:
                // nothing, keep going
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return HeapResult::OutOfHostMemory;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return HeapResult::OutOfDeviceMemory;
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
                return HeapResult::InvalidCapture;
            default: HC_UNREACHABLE("vkBindBufferMemory should not return any other VkResult values");
        }

        this->allocation_count++;

        return HeapResult::Success;
    }

    // TODO overhaul this function, make it fail if the preferred heap does not support the texture
    u32 HeapManager::alloc_texture_memory(const VolkDeviceTable &fn_table, VkDevice device, VkDeviceMemory &memory,
                                          VkDeviceSize size, Heap preferred_heap, u32 memory_type_bits) {
        HC_ASSERT(preferred_heap != Heap::MaxEnum, "Heap must be valid");

        VkMemoryAllocateInfo memory_info = {};
        memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_info.allocationSize = size;
        memory_info.memoryTypeIndex = get_memory_type_idx(preferred_heap, memory_type_bits);

        HC_ASSERT(fn_table.vkAllocateMemory(device, &memory_info, nullptr, &memory) == VK_SUCCESS,
                  "Failed to allocate device memory"); //TODO return error value

        this->allocation_count++;

        return memory_info.memoryTypeIndex;
    }

    void HeapManager::free(const VolkDeviceTable &fn_table, VkDevice device, VkDeviceMemory &memory) noexcept {
        // Having to call free from the heap manager instead of freeing the memory directly is a bit silly, but since
        // the number of allocations will be counted for debugging and profiling, may as well do it like this
        // it also falls more inline with the purpose of the heap manager
        fn_table.vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
        this->allocation_count--;
    }
}
