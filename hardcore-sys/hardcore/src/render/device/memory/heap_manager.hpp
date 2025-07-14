#pragma once

#include <core/error.hpp>
#include <util/number.hpp>

namespace hc::render::device::memory {
    enum class Heap : u8 {
        Main = 0, //!< The index of the main, device local, heap.
        Dynamic = 1, //!< The index of the dynamic heap, used to back device local resources that the host can access directly.
        Upload = 2, //!< The index of the upload heap, dedicated to host->device transfers.
        Download = 3, //!< The index of the download heap, dedicated to device->host transfers.
    };

    u8 constexpr HEAP_COUNT = 4;

    class HeapManager {
    public:
        HeapManager() = default;

        static std::expected<HeapManager, Error> create(VkPhysicalDevice physical_device);

        HeapManager(const HeapManager&) = delete;

        HeapManager& operator=(const HeapManager&) = delete;

        HeapManager(HeapManager&&) = default;

        HeapManager& operator=(HeapManager&&) = default;

        /**
        * @brief Allocate a new buffer and its backing memory.
        *
        * @param fn_table The device function table.
        * @param device The device handle.
        * @param size The size of the buffer (and its backing memory) in bytes.
        * @param usage The buffer usage flags.
        * @param heap The heap in which the backing memory will be allocated.
        * @return Heap::Success if the buffer was successfully allocated, otherwise an appropriate error value.
        */
        [[nodiscard]] std::expected<std::pair<VkDeviceMemory, VkBuffer>, Error> alloc_buffer(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            Heap heap
        ) noexcept;

        std::expected<VkDeviceMemory, Error> alloc_texture_memory(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            VkDeviceSize size,
            Heap heap,
            u32 memory_type_bits
        );

        /**
        * @brief Free the memory associated with the provided handle.
        *
        * @param fn_table The device function table.
        * @param device The device handle.
        * @param memory The handle of the memory to be freed.
        */
        void free(const VolkDeviceTable& fn_table, VkDevice device, VkDeviceMemory& memory) noexcept;

        /**
        * @brief Get the memory properties of the associated device.
        *
        * @return The memory properties struct.
        */
        [[nodiscard]] const VkPhysicalDeviceMemoryProperties& properties() const noexcept {
            return this->mem_properties;
        }

        /**
        * @brief Check the dynamic heap host coherency.
        *
        * @return *true* if writes to the dynamic heap are automatically flushed, *false* otherwise.
        */
        [[nodiscard]] inline bool host_coherent_dynamic_heap() const noexcept {
            return this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Dynamic)]].propertyFlags
                & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        /**
        * @brief Check the upload heap host coherency.
        *
        * @return *true* if writes to the upload heap are automatically flushed, *false* otherwise.
        */
        [[nodiscard]] inline bool host_coherent_upload_heap() const noexcept {
            return this->mem_properties.memoryTypes[this->heap_indexes[static_cast<Sz>(Heap::Upload)]].propertyFlags
                & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        /**
        * @brief Get the current number of allocations.
        *
        * @return The number of allocations.
        */
        [[nodiscard]] u32 allocations() const noexcept { return allocation_count; }

    private:
        [[nodiscard]] u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);
        [[nodiscard]] bool heap_meets_requirements(Heap heap, u32 memory_type_bits) const noexcept;
        [[nodiscard]] std::vector<u32> valid_heaps(u32 memory_type_bits) const noexcept;
        [[nodiscard]] std::expected<VkDeviceMemory, Error> allocate_memory(
            VolkDeviceTable const& fn_table,
            VkDevice device,
            VkMemoryAllocateInfo const& memory_info
        );

        VkPhysicalDeviceMemoryProperties mem_properties = {};

        // TODO this needs to be revised, every available heap should be usable
        std::array<u32, HEAP_COUNT> heap_indexes = {
            std::numeric_limits<u32>::max(),
            std::numeric_limits<u32>::max(),
            std::numeric_limits<u32>::max(),
            std::numeric_limits<u32>::max(),
        };

        u32 allocation_count = 0;
        //        std::array<VkDeviceSize, static_cast<Sz>(Heap::MaxEnum)> allocated_memory;
    };
}
