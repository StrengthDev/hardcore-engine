#pragma once

#include <vector>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <core/util.hpp>

// The way this is implemented is a bit overly complex, but it's done this way to avoid using virtual inheritance

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
        if (alignment)
            return (alignment - (offset % alignment)) % alignment;
        else
            return 0;
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

    struct AllocationSpec {
        VkDeviceSize size_needed;
        VkDeviceSize offset;
        u32 slot_idx;
    };

    struct Slot {
        VkDeviceSize offset; // Relative to previous slot
        VkDeviceSize size;
        bool in_use;
    };

    struct NullSlot {
    };

    struct TextureSlot {
        VkImage image;
        VkImageView view;

        VkDeviceSize size;
        VkExtent3D dims;
        VkImageLayout layout; // Layout of the image at the start of the frame
    };

    template<typename T>
    struct ExtraSlotSolver {
        static_assert(InstantiatedFalse<T>::value, "Unimplemented slot type");
    };

    template<>
    struct ExtraSlotSolver<NullSlot> {
        using type = NullSlot;
    };

    template<>
    struct ExtraSlotSolver<TextureSlot> {
        using type = std::vector<TextureSlot>;
    };

    template<typename T>
    class AllocationPool {
    public:
        AllocationPool() = default;

        [[nodiscard]] AllocationSpec search(VkDeviceSize size, VkDeviceSize alignment) const {
            const u32 invalid_idx = std::numeric_limits<u32>::max();
            const AllocationSpec invalid_spec = {
                    .size_needed = 0,
                    .offset = std::numeric_limits<VkDeviceSize>::max(),
                    .slot_idx = invalid_idx,
            };

            if (this->capacity < size)
                return invalid_spec;

            if (this->slots.size() - 1 < this->largest_free_slot || this->slots[this->largest_free_slot].in_use) {
                // Largest free slot is unknown, must check everything, look for smallest possible fit
                // if possible, update the largest free slot to reduce cost of future calls

                VkDeviceSize largest_fit = 0;
                u32 largest_idx = invalid_idx;
                VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
                u32 smallest_idx = invalid_idx;

                for (u32 i = 0; i < this->slots.size(); i++) {
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
                    }
                }

                if (smallest_idx != invalid_idx) {
                    if (largest_idx != invalid_idx && largest_idx != smallest_idx)
                        this->largest_free_slot = largest_idx;

                    return {
                            .size_needed = size + alignment_pad(this->slots[smallest_idx].offset, alignment),
                            .offset = this->slots[smallest_idx].offset,
                            .slot_idx = smallest_idx,
                    };
                } else {
                    if (largest_idx != invalid_idx)
                        this->largest_free_slot = largest_idx;

                    return invalid_spec;
                }
            } else {
                // Largest free slot is known, so if it is large enough, assign a slot in this pool,
                // otherwise move on to next pool

                if (this->slots[this->largest_free_slot].size < size)
                    return invalid_spec;

                VkDeviceSize smallest_fit = std::numeric_limits<VkDeviceSize>::max();
                u32 smallest_idx = invalid_idx;

                for (u32 i = 0; i < this->slots.size(); i++) {
                    if (!this->slots[i].in_use && smallest_fit > this->slots[i].size &&
                        size + alignment_pad(this->slots[i].offset, alignment) <= this->slots[i].size) {
                        smallest_fit = this->slots[i].size;
                        smallest_idx = i;
                    }
                }

                return {
                        .size_needed = size + alignment_pad(this->slots[smallest_idx].offset, alignment),
                        .offset = this->slots[smallest_idx].offset,
                        .slot_idx = smallest_idx,
                };
            }
        }

        /**
         * @brief Fill in an unused slot with an allocation.
         *
         * If there is still memory to spare after the slot is filled, a new empty slot with the remaining memory is
         * added after the selected slot.
         *
         * @param slot_idx The slot to be filled.
         * @param size The amount of bytes the new allocation uses.
         */
        void fill_slot(u32 slot_idx, VkDeviceSize size) {
            HC_ASSERT(slot_idx < this->slots.size(), "Slot index out of bounds");
            HC_ASSERT(!this->slots[slot_idx].in_use, "Slot must not already be in use");
            HC_ASSERT(size <= this->slots[slot_idx].size, "Allocation size greater than slot size");
            HC_ASSERT((slot_idx == 0) | this->slots[slot_idx - 1].in_use, "Slot before a selected slot must be in use");

            if (size < this->slots[slot_idx].size) {
                if (slot_idx + 1 < this->slots.size()) {
                    // Check if the next slot is already an empty slot we can use
                    if (this->slots[slot_idx + 1].size != 0) {
                        // Check if we need to insert a new empty slot, or have one already at the end that we can use
                        if (this->slots.back().size == 0)
                            rotate_right(slot_idx + 1, 1, this->slots.size() - 1);
                        else
                            insert(slot_idx + 1);
                    }
                } else {
                    insert(slot_idx + 1);
                }

                this->slots[slot_idx + 1].offset = this->slots[slot_idx].offset + size;
                this->slots[slot_idx + 1].size = this->slots[slot_idx].size - size;
                this->slots[slot_idx].size = size;
            }

            this->slots[slot_idx].in_use = true;
        }

        // TODO cleanup function to remove excessive amount of empty slots maybe

        /**
         * @brief Find the slot matching the provided memory offset.
         *
         * @param offset The memory offset of the slot to be found.
         * @return The index of the slot, if one is found. If no slot matching the offset is found, return 0xFFFFFFFF.
         */
        [[nodiscard]] u32 find_slot(VkDeviceSize offset) const noexcept {
            VkDeviceSize total_offset = 0;
            u32 i = 0;
            for (; i < this->slots.size() && total_offset < offset; i++)
                total_offset += this->slots[i].offset;

            if (total_offset == offset)
                return i;
            else
                return std::numeric_limits<u32>::max();
        }

    private:
        void insert(u32 index) {
            static_assert(InstantiatedFalse<T>::value, "Unimplemented slot type");
        }

        void rotate_right(u32 start, u32 n, u32 end) {
            static_assert(InstantiatedFalse<T>::value, "Unimplemented slot type");
        }

    protected:
        AllocationPool(VkDeviceMemory memory, VkDeviceSize size, bool per_frame_allocation = false) :
                memory(memory), capacity(size), per_frame_allocation(per_frame_allocation) {
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

        void free_memory(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
            if (this->memory != VK_NULL_HANDLE) {
                heap_manager.free(fn_table, device, this->memory);
                this->slots.clear();
            }
        }

        ExternalHandle<VkDeviceMemory, VK_NULL_HANDLE> memory;
        VkDeviceSize capacity = 0;

        std::vector<Slot> slots;

        mutable u32 largest_free_slot = std::numeric_limits<u32>::max();
        // TODO is this needed as a member variable?
        bool per_frame_allocation = false;

        using ExtraSlots = ExtraSlotSolver<T>::type;
        ExtraSlots extra_slots;
    };

    template<>
    void AllocationPool<NullSlot>::insert(u32 index) {
        this->slots.insert(this->slots.begin() + index, {});
    }

    template<>
    void AllocationPool<TextureSlot>::insert(u32 index) {
        this->slots.insert(this->slots.begin() + index, {});
        this->extra_slots.insert(this->extra_slots.begin() + index, {});
    }

    template<>
    void AllocationPool<NullSlot>::rotate_right(u32 begin, u32 n, u32 end) {
        const u32 size = this->slots.size();
        auto begin_it = this->slots.rbegin() + (size - 1 - end);
        auto middle_it = begin_it + n;
        auto end_it = this->slots.rbegin() + (size - begin);
        std::rotate(begin_it, middle_it, end_it);
    }

    template<>
    void AllocationPool<TextureSlot>::rotate_right(u32 begin, u32 n, u32 end) {
        const u32 size = this->slots.size();
        auto begin_it = this->slots.rbegin() + (size - 1 - end);
        auto middle_it = begin_it + n;
        auto end_it = this->slots.rbegin() + (size - begin);
        std::rotate(begin_it, middle_it, end_it);
        auto e_begin_it = this->extra_slots.rbegin() + (size - 1 - end);
        auto e_middle_it = e_begin_it + n;
        auto e_end_it = this->extra_slots.rbegin() + (size - begin);
        std::rotate(e_begin_it, e_middle_it, e_end_it);
    }
}
