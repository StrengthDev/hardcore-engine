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

        if (this->total_capacity < size) return invalid_spec;

        if (this->slots.size() - 1 < this->largest_free_slot || this->slots[this->largest_free_slot].in_use) {
            // Largest free slot is unknown, must check everything, look for smallest possible fit
            // if possible, update the largest free slot to reduce cost of future calls

            VkDeviceSize max_size = 0;
            u32 max_idx = invalid_idx;
            VkDeviceSize min_size = std::numeric_limits<VkDeviceSize>::max();
            u32 min_idx = invalid_idx;

            for (u32 i = 0; i < this->slots.size(); i++) {
                if (this->slots[i].in_use) continue;

                if (max_size < this->slots[i].size) {
                    max_size = this->slots[i].size;
                    max_idx = i;
                }

                if (min_size > this->slots[i].size && size + alignment_pad(this->slots[i].offset, alignment) <= this->
                    slots[i].size) {
                    min_size = this->slots[i].size;
                    min_idx = i;
                }
            }

            if (min_idx != invalid_idx) {
                if (max_idx != invalid_idx && max_idx != min_idx) this->largest_free_slot = max_idx;

                return {
                    .range = {
                        .size = size,
                        .offset = this->slots[min_idx].offset,
                        .padding = alignment_pad(this->slots[min_idx].offset, alignment),
                    },
                    .slot_idx = min_idx,
                };
            } else {
                if (max_idx != invalid_idx) this->largest_free_slot = max_idx;

                return invalid_spec;
            }
        } else {
            // Largest free slot is known, so if it is large enough, assign a slot in this pool,
            // otherwise move on to next pool

            if (this->slots[this->largest_free_slot].size < size) return invalid_spec;

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
        if (slot_idx != 0) {
            u32 left_free = slot_idx;
            while (0 < left_free && !this->slots[left_free - 1].in_use) left_free--;

            if (left_free != slot_idx) {
                this->slots[left_free].size += this->slots[slot_idx].size;
                this->slots[slot_idx].size = 0;
            }
        }
    }

    std::optional<u32> AllocationPool::find_slot(VkDeviceSize offset) const noexcept {
        u32 slot_idx = 0;

        // TODO this is probably an inefficient way to look for the slot
        for (const Slot& slot : this->slots) {
            if (offset <= slot.offset) {
                if (slot.offset == offset) {
                    if (slot.in_use) break;
                    else return std::nullopt;
                }

                slot_idx = this->slots.size();
                break;
            }

            slot_idx++;
        }

        if (slot_idx == this->slots.size()) return std::nullopt;
        else return slot_idx;
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
        }
    }
}
