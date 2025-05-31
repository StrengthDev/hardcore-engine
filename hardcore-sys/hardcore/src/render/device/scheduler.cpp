#include <pch.hpp>

#include <core/log.hpp>
#include <render/util.hpp>

#include "scheduler.hpp"

#include "util/flow.hpp"

namespace hc::render::device {
    std::expected<Scheduler, SchedulerError> Scheduler::create(VkPhysicalDevice physical_device) {
        Scheduler scheduler;
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        scheduler.queue_families = std::vector(queue_family_count, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device,
            &queue_family_count,
            scheduler.queue_families.data()
        );

        u32 compute_score = 0;
        u32 transfer_score = 0;

        for (u32 i = 0; i < scheduler.queue_families.size(); i++) {
            // Does this even happen?
            if (!scheduler.queue_families[i].queueCount) {
                continue;
            }

            if (scheduler.queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // Graphics queue is selected at runtime, depending on target surfaces
                scheduler.device_graphics_queues.push_back({VK_NULL_HANDLE, i});
            }

            if (compute_score < 1 && (scheduler.queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                scheduler.device_compute_queue.family = i;
                compute_score = 1;
            }

            if (compute_score < 2 && (scheduler.queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(scheduler.queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // Async compute queue is optimal
                scheduler.device_compute_queue.family = i;
                compute_score = 2;
            }

            if (transfer_score < 1 && (scheduler.queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                scheduler.device_transfer_queue.family = i;
                transfer_score = 1;
            }

            if (transfer_score < 2 && (scheduler.queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(scheduler.queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                scheduler.device_transfer_queue.family = i;
                transfer_score = 2;
            }

            if (transfer_score < 3 && (scheduler.queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(scheduler.queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
                // Async transfer is optimal for host<->device transfers
                scheduler.device_transfer_queue.family = i;
                transfer_score = 3;
            }

            HC_TRACE(
                "Queue family " << i << " properties: Count: " << scheduler.queue_families[i].queueCount << "\tFlags: "
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "GRAPHICS | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "COMPUTE | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "TRANSFER | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SPARSE_BINDING | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_PROTECTED_BIT ? "PROTECTED | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? "VIDEO_DECODE | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ? "VIDEO_ENCODE | " : "")
                << (scheduler.queue_families[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ? "OPTICAL_FLOW | " : "")
                << (scheduler.queue_families[i].queueFlags ? "\b\b  " : "NONE")
            );
        }

        if (scheduler.device_graphics_queues.empty()) {
            HC_ERROR("No graphics queue families found");
            return std::unexpected(SchedulerError::NoGraphicsQueueFound);
        }

        if (scheduler.device_compute_queue.family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected compute queue family index: " << scheduler.device_compute_queue.family);
        } else {
            HC_WARN("No compute queue family found");
        }

        if (scheduler.device_transfer_queue.family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected transfer queue family index: " << scheduler.device_transfer_queue.family);
        } else {
            HC_ERROR("No transfer queue family found");
            return std::unexpected(SchedulerError::NoTransferQueueFound);
        }

        return scheduler;
    }

    std::set<u32> Scheduler::unique_families() const noexcept {
        HC_ASSERT(!this->device_graphics_queues.empty(), "There must be at least 1 graphics queue");
        HC_ASSERT(this->device_transfer_queue.family != std::numeric_limits<u32>::max(), "There must be a transfer queue");

        std::set<u32> unique_families;

        for (auto const& queue : this->device_graphics_queues) {
            unique_families.insert(queue.family);
        }

        unique_families.insert(this->device_transfer_queue.family);

        if (this->device_compute_queue.family != std::numeric_limits<u32>::max()) {
            unique_families.insert(this->device_compute_queue.family);
        }

        return unique_families;
    }

    void Scheduler::init(const VkDevice& device, const VolkDeviceTable& fn_table) noexcept {
        for (u32 const queue_family : this->unique_families()) {
            VkQueue handle = VK_NULL_HANDLE;
            fn_table.vkGetDeviceQueue(device, queue_family, 0, &handle);

            for (auto& queue : this->device_graphics_queues) {
                if (queue.family == queue_family) {
                    queue.handle = handle;
                    break;
                }
            }

            if (device_compute_queue.family == queue_family) {
                device_compute_queue.handle = handle;
            }

            if (device_transfer_queue.family == queue_family) {
                device_transfer_queue.handle = handle;
            }
        }
    }

    std::optional<u32> Scheduler::present_support(
        VkPhysicalDevice const& physical_handle,
        VkSurfaceKHR const& surface
    ) const {
        for (u32 i = 0; i < this->device_graphics_queues.size(); i++) {
            u32 const family = this->device_graphics_queues[i].family;
            VkBool32 supported = VK_FALSE;
            VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_handle,
                family,
                surface,
                &supported
            );

            if (res != VK_SUCCESS) {
                HC_ERROR("Failed to query queue family surface support for queue family " << family << ": " << to_str(res));
                continue;
            }

            if (supported == VK_TRUE) {
                return i;
            }
        }

        return std::nullopt;
    }
}
