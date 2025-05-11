#pragma once

#include "heap_manager.hpp"

#include <core/glfw.hpp>

#include <util/flow.hpp>
#include <util/number.hpp>
#include <util/uncopyable.hpp>

#include <vector>

namespace hc::render::device::memory {
    /**
    * @brief This function calculates the amount of bytes necessary to add to the given offset, in order for it to be
    * aligned to the given alignment.
    *
    * @param offset Starting offset.
    * @param alignment Target memory alignment.
    * @return Number of padding bytes.
    */
    constexpr VkDeviceSize alignment_pad(VkDeviceSize offset, VkDeviceSize alignment) {
        return alignment ? (alignment - (offset % alignment)) % alignment : 0;
    }

    /**
    * @brief This function calculates a new offset, larger than the given offset, aligned to alignment.
    *
    * @param offset Starting offset.
    * @param alignment Target memory alignment.
    * @return New aligned offset.
    */
    constexpr VkDeviceSize aligned_offset(VkDeviceSize offset, VkDeviceSize alignment) {
        return offset + alignment_pad(offset, alignment);
    }

    struct PoolRange {
        VkDeviceSize size; //!< The size of the range, in bytes, without the padding.
        VkDeviceSize offset; //!< The absolute byte offset of the range.
        VkDeviceSize padding; //!< The number of padding bytes before the range's contents.
    };

    struct PoolSearchResult {
        PoolRange range;
        u64 slot_idx;
    };

    struct Slot {
        VkDeviceSize offset; //!< The absolute byte offset of the slot.
        VkDeviceSize size; //!< The total volume this slot occupies, in bytes.
        bool in_use; //!< Indicates if the slot is occupied.
    };

    class AllocationPool {
    public:
        AllocationPool() = default;

        [[nodiscard]] PoolSearchResult search(VkDeviceSize size, VkDeviceSize alignment) const;

        /**
        * @brief Allocate ´size´ bytes in this pool.
        *
        * @param size The amount of bytes to allocate.
        * @param alignment The byte alignment of the underlying buffer.
        *
        * @return An optional that will hold the allocation range, if it was successful.
        */
        [[nodiscard]] std::optional<PoolRange> allocate(VkDeviceSize size, VkDeviceSize alignment);

        /**
         * @brief Free an allocation in this pool.
         *
         * @param offset The offset to the allocation, in bytes.
         */
        void free_allocation(VkDeviceSize offset);

        // TODO cleanup function to remove excessive amount of empty slots maybe

        /**
        * @brief Find the taken slot matching the provided memory offset.
        *
        * @param offset The memory offset of the slot to be found.
        * @return The index of the slot, if one is found. If no slot matching the offset is found, return 0xFFFFFFFF.
        */
        [[nodiscard]] std::optional<u32> find_slot(VkDeviceSize offset) const noexcept;

        /**
        * @brief Get the pool's total capacity in bytes.
        *
        * @return The pool's capacity.
        */
        VkDeviceSize capacity() const noexcept { return this->total_capacity; }

    protected:
        AllocationPool(VkDeviceMemory memory, VkDeviceSize size);

        void free_memory(const VolkDeviceTable& fn_table, VkDevice device, HeapManager& heap_manager) noexcept;

        ExternalHandle<VkDeviceMemory, VK_NULL_HANDLE> memory;
        VkDeviceSize total_capacity = 0; //!< The pool's total capacity in bytes.

        std::vector<Slot> slots;

        mutable u32 largest_free_slot = std::numeric_limits<u32>::max();
    };
}
