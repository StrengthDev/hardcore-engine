#pragma once

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/util.hpp>

namespace hc::render::device::memory {
    enum class HeapResult : u8 {
        Success = 0, //!< Success.
        HeapNotFound, //!< Could not find a matching heap in the device.
        OutOfHostMemory, //!< Not enough memory to perform the allocation on the host.
        OutOfDeviceMemory, //!< Not enough memory to perform the allocation on the device.
        UnsupportedHeap, //!< The provided heap cannot back the specified buffer.
        InvalidCapture, //!< Invalid opaque capture address. (VK_ERROR_INVALID_EXTERNAL_HANDLE)
        InvalidHandle, //!< Invalid external handle. (VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR)
    };

    enum class Heap : u8 {
        Main = 0, //!< The index of the main, device local, heap.
        Dynamic = 1, //!< The index of the dynamic heap, used to back device local resources that the host can access directly.
        Upload = 2, //!< The index of the upload heap, dedicated to host->device transfers.
        Download = 3, //!< The index of the download heap, dedicated to device->host transfers.
        MaxEnum, //!< Invalid heap value.
    };

    class HeapManager {
    public:
        HeapManager() = default;

        static Result<HeapManager, HeapResult> create(VkPhysicalDevice physical_device);

        HeapManager(const HeapManager &) = delete;

        HeapManager &operator=(const HeapManager &) = delete;

        HeapManager(HeapManager &&) = default;

        HeapManager &operator=(HeapManager &&) = default;

        /**
         * @brief Allocate a new buffer and its backing memory.
         *
         * @param fn_table The device function table.
         * @param device The device handle.
         * @param memory The memory handle.
         * @param buffer The buffer handle.
         * @param size The size of the buffer (and its backing memory) in bytes.
         * @param usage The buffer usage flags.
         * @param heap The heap in which the backing memory will be allocated.
         * @return Heap::Success if the buffer was successfully allocated, otherwise an appropriate error value.
         */
        [[nodiscard]] HeapResult alloc_buffer(const VolkDeviceTable &fn_table, VkDevice device,
                                              VkDeviceMemory &memory, VkBuffer &buffer,
                                              VkDeviceSize size, VkBufferUsageFlags usage, Heap heap) noexcept;

        u32 alloc_texture_memory(const VolkDeviceTable &fn_table, VkDevice device, VkDeviceMemory &memory,
                                 VkDeviceSize size,
                                 Heap preferred_heap, u32 memory_type_bits);

        /**
         * @brief Free the memory associated with the provided handle.
         *
         * @param fn_table The device function table.
         * @param device The device handle.
         * @param memory The handle of the memory to be freed.
         */
        void free(const VolkDeviceTable &fn_table, VkDevice device, VkDeviceMemory &memory) noexcept;

        /**
         * @brief Get the memory properties of the associated device.
         *
         * @return The memory properties struct.
         */
        [[nodiscard]] const VkPhysicalDeviceMemoryProperties &properties() const noexcept {
            return this->mem_properties;
        }

        /**
         * @brief Check the dynamic heap host coherency.
         *
         * @return *true* if writes to the dynamic heap are automatically flushed, *false* otherwise.
         */
        [[nodiscard]] inline bool host_coherent_dynamic_heap() const noexcept {
            return this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Dynamic)]].propertyFlags &
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        /**
         * @brief Check the upload heap host coherency.
         *
         * @return *true* if writes to the upload heap are automatically flushed, *false* otherwise.
         */
        [[nodiscard]] inline bool host_coherent_upload_heap() const noexcept {
            return this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Upload)]].propertyFlags &
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        /**
         * @brief Get the current number of allocations.
         *
         * @return The number of allocations.
         */
        [[nodiscard]] u32 allocations() const noexcept { return allocation_count; }

    private:
        [[nodiscard]] u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);

        [[nodiscard]] u32 get_memory_type_idx(Heap heap, u32 memory_type_bits) const;

        VkPhysicalDeviceMemoryProperties mem_properties = {};

        std::array<u32, static_cast<Sz>(Heap::MaxEnum)> heap_indexes = {
                std::numeric_limits<u32>::max(),
                std::numeric_limits<u32>::max(),
                std::numeric_limits<u32>::max(),
                std::numeric_limits<u32>::max(),
        };

        u32 allocation_count = 0;
//        std::array<VkDeviceSize, static_cast<Sz>(Heap::MaxEnum)> allocated_memory;
    };
}
