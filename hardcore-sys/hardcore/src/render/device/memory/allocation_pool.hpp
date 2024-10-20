#pragma once

#include <vector>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <util/flow.hpp>
#include <util/number.hpp>
#include <util/trait.hpp>

// The way this is implemented is a bit overly complex, but it's done this way to avoid using virtual inheritance

namespace hc::render::device::memory {
	const u32 INITIAL_SLOTS = 16;

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
		VkDeviceSize size;
		VkDeviceSize offset;
		VkDeviceSize padding;
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
				.size = 0,
				.offset = std::numeric_limits<VkDeviceSize>::max(),
				.padding = std::numeric_limits<VkDeviceSize>::max(),
				.slot_idx = invalid_idx,
			};

			if (this->total_capacity < size)
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
						.size = size,
						.offset = this->slots[smallest_idx].offset,
						.padding = alignment_pad(this->slots[smallest_idx].offset, alignment),
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
					.size = size,
					.offset = this->slots[smallest_idx].offset,
					.padding = alignment_pad(this->slots[smallest_idx].offset, alignment),
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
			HC_ASSERT((slot_idx == 0) || this->slots[slot_idx - 1].in_use,
					"Slot before a selected slot must be in use");

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

		void clear_slot(VkDeviceSize offset) {
			auto slot_opt = find_slot(offset);
			HC_ASSERT(slot_opt, "A slot with the given offset should exist");
			u32 slot_idx = *slot_opt;

			this->slots[slot_idx].in_use = false;

			// Take the space of the free slot directly after, if there is any
			if (slot_idx + 1 < this->slots.size() && !this->slots[slot_idx + 1].in_use &&
				this->slots[slot_idx + 1].size) {
				this->slots[slot_idx].size += this->slots[slot_idx + 1].size;
				this->slots[slot_idx + 1].size = 0;
			}

			// If there is free space before the slot, take the space from the freed slot and put it there
			if (slot_idx != 0) {
				u32 left_free = slot_idx;
				while (0 < left_free && !this->slots[left_free - 1].in_use)
					left_free--;

				if (left_free != slot_idx) {
					this->slots[left_free].size += this->slots[slot_idx].size;
					this->slots[slot_idx].size = 0;
				}
			}
		}

		// TODO cleanup function to remove excessive amount of empty slots maybe

		/**
		* @brief Find the taken slot matching the provided memory offset.
		*
		* @param offset The memory offset of the slot to be found.
		* @return The index of the slot, if one is found. If no slot matching the offset is found, return 0xFFFFFFFF.
		*/
		[[nodiscard]] std::optional<u32> find_slot(VkDeviceSize offset) const noexcept {
			u32 slot_idx = 0;

			// TODO this is probably an inefficient way to look for the slot
			for (const Slot &slot: this->slots) {
				if (offset <= slot.offset) {
					if (slot.offset == offset) {
						if (slot.in_use)
							break;
						else
							return std::nullopt;
					}

					slot_idx = this->slots.size();
					break;
				}

				slot_idx++;
			}

			if (slot_idx == this->slots.size())
				return std::nullopt;
			else
				return slot_idx;
		}

		/**
		* @brief Get the pool's total capacity in bytes.
		*
		* @return The pool's capacity.
		*/
		VkDeviceSize capacity() const noexcept {
			return this->total_capacity;
		}

	private:
		void insert(u32 index) {
			this->slots.insert(this->slots.begin() + index, {});

			if constexpr (is_std_vector<ExtraSlots>::value)
				this->extra_slots.insert(this->extra_slots.begin() + index, {});
		}

		void rotate_right(u32 begin, u32 n, u32 end) {
			const u32 size = this->slots.size();

			auto begin_it = this->slots.rbegin() + (size - 1 - end);
			auto middle_it = begin_it + n;
			auto end_it = this->slots.rbegin() + (size - begin);
			std::rotate(begin_it, middle_it, end_it);

			if constexpr (is_std_vector<ExtraSlots>::value) {
				auto e_begin_it = this->extra_slots.rbegin() + (size - 1 - end);
				auto e_middle_it = e_begin_it + n;
				auto e_end_it = this->extra_slots.rbegin() + (size - begin);
				std::rotate(e_begin_it, e_middle_it, e_end_it);
			}
		}

	protected:
		AllocationPool(VkDeviceMemory memory, VkDeviceSize size, bool per_frame_allocation = false) : memory(memory),
			total_capacity(size), slots(INITIAL_SLOTS), per_frame_allocation(per_frame_allocation) {
			this->slots[0].in_use = false;
			this->slots[0].offset = 0;
			this->slots[0].size = size;
			this->largest_free_slot = 0;

			for (u32 i = 1; i < this->slots.size(); i++) {
				this->slots[i].in_use = false;
				this->slots[i].offset = size;
				this->slots[i].size = 0;
			}

			if constexpr (is_std_vector<ExtraSlots>::value)
				this->extra_slots = ExtraSlotSolver<T>::type(INITIAL_SLOTS);
		}

		void free_memory(const VolkDeviceTable &fn_table, VkDevice device, HeapManager &heap_manager) noexcept {
			if (this->memory != VK_NULL_HANDLE) {
				heap_manager.free(fn_table, device, this->memory);
				this->slots.clear();
			}
		}

		ExternalHandle<VkDeviceMemory, VK_NULL_HANDLE> memory;
		VkDeviceSize total_capacity = 0; //!< The pool's total capacity in bytes.

		std::vector<Slot> slots;

		mutable u32 largest_free_slot = std::numeric_limits<u32>::max();
		// TODO is this needed as a member variable?
		bool per_frame_allocation = false;

		using ExtraSlots = ExtraSlotSolver<T>::type;
		ExtraSlots extra_slots;
	};
}
