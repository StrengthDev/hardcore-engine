#pragma once

#include <util/number.hpp>

namespace hc::render::device {
    struct Queue {
        VkQueue handle = VK_NULL_HANDLE;
        u32 family = std::numeric_limits<u32>::max();
    };

    enum class SchedulerError {
        NoGraphicsQueueFound,
        NoTransferQueueFound,
    };

    class Scheduler {
    public:
        static std::expected<Scheduler, SchedulerError> create(VkPhysicalDevice physical_device);

        Scheduler() = default;

        Scheduler(const Scheduler&) = delete;

        Scheduler& operator=(const Scheduler&) = delete;

        Scheduler(Scheduler&& other) noexcept = default;

        Scheduler& operator=(Scheduler&& other) noexcept = default;

        std::set<u32> unique_families() const noexcept;

        void init(const VkDevice& device, const VolkDeviceTable& fn_table) noexcept;

        [[nodiscard]] std::optional<u32> present_support(
            const VkPhysicalDevice& physical_handle,
            const VkSurfaceKHR& surface
        ) const;

        [[nodiscard]] std::vector<Queue> const& graphics_queues() const noexcept { return this->device_graphics_queues; }

        [[nodiscard]] Queue const& compute_queue() const noexcept { return this->device_compute_queue; }

        [[nodiscard]] Queue const& transfer_queue() const noexcept { return this->device_transfer_queue; }

    private:
        std::vector<VkQueueFamilyProperties> queue_families; //!< The properties of a device's queue families.

        std::vector<Queue> device_graphics_queues;
        Queue device_compute_queue;
        Queue device_transfer_queue;
    };
}
