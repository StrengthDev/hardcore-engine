#pragma once

#include <core/error.hpp>
#include <util/number.hpp>
#include "util/uncopyable.hpp"

namespace hc::render::device {
    struct QueueSelection {
        std::vector<VkQueueFamilyProperties> family_properties;
        std::vector<u32> graphics_families;
        u32 compute_family;
        u32 transfer_family;
    };

    struct CommandPool {
        ExternalHandle<VkCommandPool, VK_NULL_HANDLE> handle;
        ExternalHandle<VkCommandBuffer, VK_NULL_HANDLE> buffer;
        ExternalHandle<VkFence, VK_NULL_HANDLE> fence;
        ExternalHandle<VkSemaphore, VK_NULL_HANDLE> semaphore;

        static std::expected<CommandPool, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            u32 queue_family
        );

        void destroy(const VolkDeviceTable& fn_table, VkDevice device);
    };

    struct Queue {
        ExternalHandle<VkQueue, VK_NULL_HANDLE> handle;
        u32 family = std::numeric_limits<u32>::max();
        VkQueueFamilyProperties family_properties;
        std::vector<CommandPool> pools;
    };

    class Scheduler {
    public:
        static std::expected<QueueSelection, Error> select_queues(VkPhysicalDevice physical_device);

        Scheduler() = default;

        static std::expected<Scheduler, Error> create(
            const VolkDeviceTable& fn_table,
            VkDevice device,
            QueueSelection const& selection,
            u8 max_frames_in_flight
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

        std::expected<void, Error> reset_pools(const VolkDeviceTable& fn_table, VkDevice device, u8 frame_mod);

    private:
        std::vector<Queue> queues;

        std::vector<u32> graphics_queue_indexes;
        u32 compute_queue_index = std::numeric_limits<u32>::max();
        u32 transfer_queue_index = std::numeric_limits<u32>::max();

        std::vector<std::reference_wrapper<const Queue>> graphics_queue_refs;
    };
}
