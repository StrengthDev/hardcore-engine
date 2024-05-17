#include <pch.hpp>

#include <core/log.hpp>
#include <render/vars.hpp>
#include <render/util.hpp>

#include "scheduler.hpp"

namespace hc::render::device {

    void Scheduler::select_queue_families(const std::vector<VkQueueFamilyProperties> &queue_families,
                                          std::vector<u32> &out_graphics_queue_families, u32 &out_compute_idx,
                                          u32 &out_transfer_idx) {
        u32 compute_idx = out_compute_idx;
        u32 transfer_idx = out_transfer_idx;
        u32 compute_score = 0;
        u32 transfer_score = 0;

        for (u32 i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].queueCount) {
                if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    // Graphics queue is selected at runtime, depending on target surfaces
                    out_graphics_queue_families.push_back(i);
                }

                if (compute_score < 1 && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    compute_idx = i;
                    compute_score = 1;
                }

                if (compute_score < 2 && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                    !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    // Async compute queue is optimal
                    compute_idx = i;
                    compute_score = 2;
                }

                if (transfer_score < 1 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                    transfer_idx = i;
                    transfer_score = 1;
                }

                if (transfer_score < 2 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    transfer_idx = i;
                    transfer_score = 2;
                }

                if (transfer_score < 3 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    !(queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
                    // Async transfer is optimal
                    transfer_idx = i;
                    transfer_score = 3;
                }

                HC_TRACE(
                        "Queue family " << i << " properties:\tCount: " << queue_families[i].queueCount << "\tFlags: "
                                        //<< '(' << std::bitset<sizeof(VkQueueFlags) * 8>(queue_families[i].queueFlags) << ") => "
                                        << (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT
                                            ? "GRAPHICS | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT
                                            ? "COMPUTE | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT
                                            ? "TRANSFER | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT
                                            ? "SPARSE_BINDING | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_PROTECTED_BIT
                                            ? "PROTECTED | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR
                                            ? "VIDEO_DECODE | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR
                                            ? "VIDEO_ENCODE | " : "")
                                        << (queue_families[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV
                                            ? "OPTICAL_FLOW | " : "")
                                        << (queue_families[i].queueFlags ? "\b\b  " : ""));
            }
        }

        out_compute_idx = compute_idx;
        out_transfer_idx = transfer_idx;
    }

    std::optional<Scheduler> Scheduler::create(VkDevice const &device, const VolkDeviceTable &fn_table, u32 parallelism,
                                               std::vector<VkQueueFamilyProperties> &&queue_families,
                                               std::set<u32> &&unique_queue_families,
                                               std::vector<u32> &&graphics_queue_families, u32 compute_family,
                                               u32 transfer_family) {
        Scheduler scheduler;

        scheduler.queue_families = std::move(queue_families);
        scheduler.graphics_queue_families = std::move(graphics_queue_families);
        scheduler.compute_family = compute_family;
        scheduler.transfer_family = transfer_family;

        for (u32 queue_family: unique_queue_families) {
            VkQueue queue = VK_NULL_HANDLE;
            fn_table.vkGetDeviceQueue(device, queue_family, 0, &queue);
            if (queue == VK_NULL_HANDLE) {
                return std::nullopt;
            }
            if (scheduler.compute_family == queue_family) {
                scheduler.compute_index = scheduler.queues.size();
            }
            if (scheduler.transfer_family == queue_family) {
                scheduler.transfer_index = scheduler.queues.size();
            }
            scheduler.queues.emplace_back(queue_family, queue);
        }

        for (u32 queue_family: scheduler.graphics_queue_families) {
            for (u32 i = 0; i < scheduler.queues.size(); i++) {
                if (queue_family == scheduler.queues[i].first) {
                    scheduler.graphics_queue_indexes.push_back(i);
                    break;
                }
            }
        }

        return scheduler;
    }

    Scheduler::Scheduler(Scheduler &&other) noexcept:
            queue_families(std::move(other.queue_families)),
            queues(std::move(other.queues)),
            graphics_queue_families(std::move(other.graphics_queue_families)),
            compute_family(std::exchange(other.compute_family, std::numeric_limits<u32>::max())),
            transfer_family(std::exchange(other.transfer_family, std::numeric_limits<u32>::max())),
            graphics_queue_indexes(std::move(other.graphics_queue_indexes)),
            compute_index(std::exchange(other.compute_index, std::numeric_limits<u32>::max())),
            transfer_index(std::exchange(other.transfer_index, std::numeric_limits<u32>::max())) {

    }

    Scheduler &Scheduler::operator=(Scheduler &&other) noexcept {
        this->queue_families = std::move(other.queue_families);
        this->queues = std::move(other.queues);
        this->graphics_queue_families = std::move(other.graphics_queue_families);
        this->compute_family = std::exchange(other.compute_family, std::numeric_limits<u32>::max());
        this->transfer_family = std::exchange(other.transfer_family, std::numeric_limits<u32>::max());
        this->graphics_queue_indexes = std::move(other.graphics_queue_indexes);
        this->compute_index = std::exchange(other.compute_index, std::numeric_limits<u32>::max());
        this->transfer_index = std::exchange(other.transfer_index, std::numeric_limits<u32>::max());

        return *this;
    }

    std::pair<u32, u32>
    Scheduler::present_support(VkPhysicalDevice const &physical_handle, VkSurfaceKHR const &surface) const {
        u32 found = std::numeric_limits<u32>::max();
        for (u32 i = 0; i < this->queues.size(); i++) {
            VkBool32 supported = VK_FALSE;
            VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physical_handle, this->queues[i].first, surface,
                                                                &supported);
            if (res != VK_SUCCESS) {
                HC_ERROR("Failed to query queue family surface support: " << to_str(res));
                break;
            }
            if (supported == VK_TRUE) {
                if (this->queue_families[this->queues[i].first].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    return std::pair(i, i);
                } else if (found == std::numeric_limits<u32>::max()) {
                    found = i;
                }
            }
        }

        if (this->graphics_queue_families.empty()) {
            return std::pair(std::numeric_limits<u32>::max(), found);
        } else {
            return std::pair(this->graphics_queue_families[0], found);
        }
    }
}
