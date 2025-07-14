#include <pch.hpp>

#include "heap_manager.hpp"

#include <render/util.hpp>
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
    std::expected<HeapManager, Error> HeapManager::create(VkPhysicalDevice physical_device) {
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
        if (missing_heaps) {
            return Error(HCError_UnmetHeapRequirements);
        }

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

        return std::move(manager);
    }

    std::expected<std::pair<VkDeviceMemory, VkBuffer>, Error> HeapManager::alloc_buffer(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        Heap heap
    ) noexcept {
        VkBuffer buffer = VK_NULL_HANDLE;

        VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkResult result = fn_table.vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to create buffer: " << to_str(result));
            return Error(result);
        }

        VkMemoryRequirements memory_requirements;
        fn_table.vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
        if (!this->heap_meets_requirements(heap, memory_requirements.memoryTypeBits)) {
            HC_ERROR("Requested heap does not satisfy requirements");
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            return Error(HCError_UnmetHeapRequirements);
        }

        VkMemoryAllocateInfo memory_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = this->heap_indexes[static_cast<Sz>(heap)],
        };

        auto memory_result = this->allocate_memory(fn_table, device, memory_info);
        if (!memory_result) {
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return memory_result.error();
        }
        VkDeviceMemory memory = *memory_result;

        result = fn_table.vkBindBufferMemory(device, buffer, memory, 0);
        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to bind buffer memory: " << to_str(result));
            this->free(fn_table, device, memory);
            fn_table.vkDestroyBuffer(device, buffer, nullptr);
            return Error(result);
        }

        return std::pair(memory, buffer);
    }

    std::expected<VkDeviceMemory, Error> HeapManager::alloc_texture_memory(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        VkDeviceSize size,
        Heap heap,
        u32 memory_type_bits
    ) {
        if (!this->heap_meets_requirements(heap, memory_type_bits)) {
            HC_ERROR("Requested heap does not satisfy requirements");
            return Error(HCError_UnmetHeapRequirements);
        }

        VkMemoryAllocateInfo memory_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = size,
            .memoryTypeIndex = this->heap_indexes[static_cast<Sz>(heap)],
        };

        auto result = this->allocate_memory(fn_table, device, memory_info);
        if (!result) {
            return result.error();
        }

        return *result;
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

    bool HeapManager::heap_meets_requirements(Heap heap, u32 memory_type_bits) const noexcept {
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

    std::expected<VkDeviceMemory, Error> HeapManager::allocate_memory(VolkDeviceTable const& fn_table, VkDevice device, VkMemoryAllocateInfo const& memory_info) {
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkResult result = fn_table.vkAllocateMemory(device, &memory_info, nullptr, &memory);

        if (result != VK_SUCCESS) {
            HC_ERROR("Failed to allocate memory: " << to_str(result));
            return Error(result);
        }

        this->allocation_count++;

        return memory;
    }
}
