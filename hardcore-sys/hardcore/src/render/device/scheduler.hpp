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

    private:
        std::vector<VkQueueFamilyProperties> queue_families;

        std::vector<std::pair<u32, VkQueue>> queues;

        std::vector<u32> graphics_queue_families;
        u32 compute_family = std::numeric_limits<u32>::max();
        u32 transfer_family = std::numeric_limits<u32>::max();
        std::vector<u32> graphics_queue_indexes;
        u32 compute_idx = std::numeric_limits<u32>::max();
        u32 transfer_idx = std::numeric_limits<u32>::max();

    };
}
