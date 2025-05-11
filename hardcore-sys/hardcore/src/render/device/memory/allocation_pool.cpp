#include <pch.hpp>

#include "allocation_pool.hpp"

namespace hc::render::device::memory {
    static constexpr u32 INITIAL_SLOTS = 16;

    PoolSearchResult AllocationPool::search(VkDeviceSize size, VkDeviceSize alignment) const {
        constexpr u32 invalid_idx = std::numeric_limits<u32>::max();
        constexpr PoolSearchResult invalid_spec = {
            .range = {
                .size = 0,
                .offset = std::numeric_limits<VkDeviceSize>::max(),
                .padding = std::numeric_limits<VkDeviceSize>::max(),
            },
            .slot_idx = invalid_idx,
        };

        if (this->total_capacity < size) {
            return invalid_spec;
        }

        if (this->slots.size() - 1 < this->largest_free_slot || this->slots[this->largest_free_slot].in_use) {
            // Largest free slot is unknown, must check everything, look for smallest possible fit
            // if possible, update the largest free slot to reduce cost of future calls

            VkDeviceSize max_size = 0;
            u32 max_idx = invalid_idx;
            VkDeviceSize min_size = std::numeric_limits<VkDeviceSize>::max();
            u32 min_idx = invalid_idx;

            for (u32 i = 0; i < this->slots.size(); i++) {
                if (this->slots[i].in_use) {
                    continue;
                }

                if (max_size < this->slots[i].size) {
                    max_size = this->slots[i].size;
                    max_idx = i;
                }

                if (min_size > this->slots[i].size
                    && size + alignment_pad(this->slots[i].offset, alignment) <= this->slots[i].size) {
                    min_size = this->slots[i].size;
                    min_idx = i;
                }
            }

            if (min_idx != invalid_idx) {
                if (max_idx != invalid_idx && max_idx != min_idx) {
                    this->largest_free_slot = max_idx;
                }

                return {
                    .range = {
                        .size = size,
                        .offset = this->slots[min_idx].offset,
                        .padding = alignment_pad(this->slots[min_idx].offset, alignment),
                    },
                    .slot_idx = min_idx,
                };
            } else {
                if (max_idx != invalid_idx) {
                    this->largest_free_slot = max_idx;
                }

                return invalid_spec;
            }
        } else {
            // Largest free slot is known, so if it is large enough, assign a slot in this pool,
            // otherwise move on to next pool

            if (this->slots[this->largest_free_slot].size < size) {
                return invalid_spec;
            }

            VkDeviceSize min_size = std::numeric_limits<VkDeviceSize>::max();
            u32 min_idx = invalid_idx;

            for (u32 i = 0; i < this->slots.size(); i++) {
                if (!this->slots[i].in_use && min_size > this->slots[i].size && size + alignment_pad(
                    this->slots[i].offset,
                    alignment
                ) <= this->slots[i].size) {
                    min_size = this->slots[i].size;
                    min_idx = i;
                }
            }

            return {
                .range = {
                    .size = size,
                    .offset = this->slots[min_idx].offset,
                    .padding = alignment_pad(this->slots[min_idx].offset, alignment),
                },
                .slot_idx = min_idx,
            };
        }
    }

    std::optional<PoolRange> AllocationPool::allocate(VkDeviceSize size, VkDeviceSize alignment) {
        auto const [range, slot_idx] = this->search(size, alignment);

        if (!range.size) {
            return std::nullopt;
        }

        size += range.padding;

        HC_ASSERT(slot_idx < this->slots.size(), "Slot index out of bounds");
        HC_ASSERT(!this->slots[slot_idx].in_use, "Slot must not already be in use");
        HC_ASSERT(size <= this->slots[slot_idx].size, "Allocation size greater than slot size");
        HC_ASSERT(slot_idx == 0 || this->slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use");

        if (size < this->slots[slot_idx].size) {
            if (slot_idx + 1 < this->slots.size()) {
                // Check if the next slot is already an empty slot we can use
                if (this->slots[slot_idx + 1].size != 0) {
                    // Check if we need to insert a new empty slot, or have one already at the end that we can use
                    if (this->slots.back().size == 0) {
                        auto begin_it = this->slots.rbegin() + (this->slots.size() - slot_idx - 2);
                        auto middle_it = begin_it + 1;
                        auto end_it = this->slots.rbegin() + 1;

                        std::rotate(begin_it, middle_it, end_it);
                    } else {
                        this->slots.emplace(this->slots.begin() + slot_idx + 1);
                    }
                }
            } else {
                this->slots.emplace(this->slots.begin() + (slot_idx + 1));
            }

            this->slots[slot_idx + 1].offset = this->slots[slot_idx].offset + size;
            this->slots[slot_idx + 1].size = this->slots[slot_idx].size - size;
            this->slots[slot_idx].size = size;
        }

        this->slots[slot_idx].in_use = true;

        return range;
    }

    void AllocationPool::free_allocation(VkDeviceSize offset) {
        auto slot_opt = find_slot(offset);
        HC_ASSERT(slot_opt, "A slot with the given offset should exist");
        u32 slot_idx = *slot_opt;

        this->slots[slot_idx].in_use = false;

        // Take the space of the free slot directly after, if there is any
        if (slot_idx + 1 < this->slots.size() && !this->slots[slot_idx + 1].in_use && this->slots[slot_idx + 1].size) {
            this->slots[slot_idx].size += this->slots[slot_idx + 1].size;
            this->slots[slot_idx + 1].size = 0;
        }

        // If there is free space before the slot, take the space from the freed slot and put it there
        if (0 < slot_idx) {
            u32 left_free = slot_idx;
            while (0 < left_free && !this->slots[left_free - 1].in_use) {
                left_free--;
            }

            if (left_free != slot_idx) {
                this->slots[left_free].size += this->slots[slot_idx].size;
                this->slots[slot_idx].size = 0;

                if (0 < left_free) {
                    this->slots[left_free].offset = this->slots[left_free - 1].offset + this->slots[left_free - 1].size;
                }
            }
        }
    }

    std::optional<u32> AllocationPool::find_slot(VkDeviceSize offset) const noexcept {
        HC_ASSERT(
            this->slots.size() <= std::numeric_limits<u32>::max(),
            "Number of slots should never go over the u32 max"
        );
        // TODO this is probably an inefficient way to look for the slot

        for (u32 i = 0; i < this->slots.size(); ++i) {
            if (offset == this->slots[i].offset) {
                return i;
            }

            if (offset < this->slots[i].offset) {
                break;
            }
        }

        return std::nullopt;
    }

    AllocationPool::AllocationPool(VkDeviceMemory memory, VkDeviceSize size)
        : memory(memory), total_capacity(size), slots(INITIAL_SLOTS) {
        this->slots[0].in_use = false;
        this->slots[0].offset = 0;
        this->slots[0].size = size;
        this->largest_free_slot = 0;

        for (u32 i = 1; i < this->slots.size(); i++) {
            this->slots[i].in_use = false;
            this->slots[i].offset = size;
            this->slots[i].size = 0;
        }
    }

    void AllocationPool::free_memory(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        HeapManager& heap_manager
    ) noexcept {
        if (this->memory != VK_NULL_HANDLE) {
            heap_manager.free(fn_table, device, this->memory);
            this->slots.clear();
            this->memory.destroy();
        }
    }
}
