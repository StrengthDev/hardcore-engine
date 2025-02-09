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
        VkDeviceSize size;
        VkDeviceSize offset;
        VkDeviceSize padding;
    };

    struct PoolSearchResult {
        PoolRange range;
        u32 slot_idx;
    };

    struct Slot {
        VkDeviceSize offset; // Relative to previous slot
        VkDeviceSize size;
        bool in_use;
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
        template <typename Self>
        std::optional<PoolRange> allocate(this Self&& self, VkDeviceSize size, VkDeviceSize alignment) {
            auto const [range, slot_idx] = self.search(size, alignment);

            if (!range.size) return std::nullopt;

            size += range.padding;

            HC_ASSERT(slot_idx < self.slots.size(), "Slot index out of bounds");
            HC_ASSERT(!self.slots[slot_idx].in_use, "Slot must not already be in use");
            HC_ASSERT(size <= self.slots[slot_idx].size, "Allocation size greater than slot size");
            HC_ASSERT(slot_idx == 0 || self.slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use");

            if (size < self.slots[slot_idx].size) {
                if (slot_idx + 1 < self.slots.size()) {
                    // Check if the next slot is already an empty slot we can use
                    if (self.slots[slot_idx + 1].size != 0) {
                        // Check if we need to insert a new empty slot, or have one already at the end that we can use
                        if (self.slots.back().size == 0) self.rotate_right(slot_idx + 1, 1, self.slots.size() - 1);
                        else self.insert(slot_idx + 1);
                    }
                } else {
                    self.insert(slot_idx + 1);
                }

                self.slots[slot_idx + 1].offset = self.slots[slot_idx].offset + size;
                self.slots[slot_idx + 1].size = self.slots[slot_idx].size - size;
                self.slots[slot_idx].size = size;
            }

            self.slots[slot_idx].in_use = true;

            return range;
        }

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
