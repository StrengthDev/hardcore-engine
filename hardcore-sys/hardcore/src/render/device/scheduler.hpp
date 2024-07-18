#pragma once

#include <optional>

#include <core/util.hpp>

namespace hc::render::device {
    class Scheduler {
    public:
        static void select_queue_families(const std::vector<VkQueueFamilyProperties> &queue_families,
                                          std::vector<u32> &out_graphics_queue_families,
                                          u32 &out_compute_idx, u32 &out_transfer_idx);

        Scheduler(const Scheduler &) = delete;

        Scheduler &operator=(const Scheduler &) = delete;

        static std::optional<Scheduler>
        create(const VkDevice &device, const VolkDeviceTable &fn_table, u32 parallelism,
               std::vector<VkQueueFamilyProperties> &&queue_families,
               std::set<u32> &&unique_queue_families,
               std::vector<u32> &&graphics_queue_families,
               u32 compute_family, u32 transfer_family);

        Scheduler() = default;

//        ~Scheduler();
//
//        void destroy();

        Scheduler(Scheduler &&other) noexcept;

        Scheduler &operator=(Scheduler &&other) noexcept;

        [[nodiscard]] std::pair<u32, u32>
        present_support(const VkPhysicalDevice &physical_handle, const VkSurfaceKHR &surface) const;

        [[nodiscard]] inline u32 graphics_queue_family() const noexcept { return this->graphics_queue_families[0]; }

        [[nodiscard]] inline u32 compute_queue_family() const noexcept { return this->compute_family; }

        [[nodiscard]] inline u32 transfer_queue_family() const noexcept { return this->transfer_family; }

    private:
        std::vector<VkQueueFamilyProperties> queue_families; //!< The properties of a device's queue families.

        std::vector<std::pair<u32, VkQueue>> queues; //!< The instanced queues, the first item of the pair is the queue's family.

        std::vector<u32> graphics_queue_families;
        u32 compute_family = std::numeric_limits<u32>::max();
        u32 transfer_family = std::numeric_limits<u32>::max();
        std::vector<u32> graphics_queue_indexes;
        u32 compute_index = std::numeric_limits<u32>::max();
        u32 transfer_index = std::numeric_limits<u32>::max();

    };
}
