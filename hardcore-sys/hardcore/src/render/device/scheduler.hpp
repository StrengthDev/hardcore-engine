#pragma once

#include "util/uncopyable.hpp"

#include <util/number.hpp>

namespace hc::render::device {
    enum class SchedulerError {
        NoGraphicsQueueFound,
        NoComputeQueueFound,
        NoTransferQueueFound,
        OutOfHostMemory,
        OutOfDeviceMemory,
    };

    struct QueueSelection {
        std::vector<VkQueueFamilyProperties> family_properties;
        std::vector<u32> graphics_families;
        u32 compute_family;
        u32 transfer_family;
    };

    struct Queue {
        ExternalHandle<VkQueue, VK_NULL_HANDLE> handle;
        u32 family = std::numeric_limits<u32>::max();
        VkQueueFamilyProperties family_properties;
        ExternalHandle<VkCommandPool, VK_NULL_HANDLE> pool;
        ExternalHandle<VkCommandBuffer, VK_NULL_HANDLE> buffer;
    };

    class Scheduler {
    public:
        static std::expected<QueueSelection, SchedulerError> select_queues(VkPhysicalDevice physical_device);

        Scheduler() = default;

        static std::expected<Scheduler, SchedulerError> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            QueueSelection const& selection
        );

        void destroy(VolkDeviceTable const& fn_table, VkDevice device);

        Scheduler(const Scheduler&) = delete;

        Scheduler& operator=(const Scheduler&) = delete;

        Scheduler(Scheduler&& other) noexcept = default;

        Scheduler& operator=(Scheduler&& other) noexcept = default;

        [[nodiscard]] std::optional<u32> present_support(
            const VkPhysicalDevice& physical_handle,
            const VkSurfaceKHR& surface
        ) const;

        [[nodiscard]] std::vector<std::reference_wrapper<const Queue>> const& graphics_queues() const noexcept {
            return this->graphics_queue_refs;
        }

        [[nodiscard]] Queue const& compute_queue() const noexcept { return this->queues[this->compute_queue_index]; }

        [[nodiscard]] Queue const& transfer_queue() const noexcept { return this->queues[this->transfer_queue_index]; }

    private:
        std::vector<Queue> queues;

        std::vector<u32> graphics_queue_indexes;
        u32 compute_queue_index;
        u32 transfer_queue_index;

        std::vector<std::reference_wrapper<const Queue>> graphics_queue_refs;
    };
}
