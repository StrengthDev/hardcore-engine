#include <pch.hpp>

#include <core/log.hpp>
#include <render/util.hpp>

#include "scheduler.hpp"

namespace hc::render::device {
    std::expected<CommandPool, Error> CommandPool::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        u32 queue_family
    ) {
        CommandPool pool;

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family,
        };

        auto pool_result = vk::CommandPool::create(fn_table, device, &pool_info);
        if (!pool_result) {
            return pool_result.error();
        }
        pool.handle = *std::move(pool_result);

        VkCommandBufferAllocateInfo command_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool.handle,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        auto buffer_result = vk::CommandBuffers::create(fn_table, device, &command_buffer_info);
        if (!buffer_result) {
            pool.destroy(fn_table, device);
            return buffer_result.error();
        }
        pool.buffers = *std::move(buffer_result);

        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        auto fence_result = vk::Fence::create(fn_table, device, &fence_info);
        if (!fence_result) {
            pool.destroy(fn_table, device);
            return fence_result.error();
        }
        pool.fence = *std::move(fence_result);

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };

        auto semaphore_result = vk::Semaphore::create(fn_table, device, &semaphore_info);
        if (!semaphore_result) {
            pool.destroy(fn_table, device);
            return semaphore_result.error();
        }
        pool.semaphore = *std::move(semaphore_result);

        return pool;
    }

    void CommandPool::destroy(const VolkDeviceTable& fn_table, VkDevice device) {
        this->semaphore.destroy(fn_table, device);
        this->fence.destroy(fn_table, device);
        this->buffers.destroy(fn_table, device, this->handle);
        this->handle.destroy(fn_table, device);
    }

    std::expected<QueueSelection, Error> Scheduler::select_queues(VkPhysicalDevice physical_device) {
        QueueSelection selection;
        selection.compute_family = std::numeric_limits<u32>::max();
        selection.transfer_family = std::numeric_limits<u32>::max();

        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        selection.family_properties = std::vector(queue_family_count, VkQueueFamilyProperties{});
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device,
            &queue_family_count,
            selection.family_properties.data()
        );

        u32 compute_score = 0;
        u32 transfer_score = 0;

        for (u32 family = 0; family < selection.family_properties.size(); family++) {
            // Does this even happen?
            if (!selection.family_properties[family].queueCount) {
                continue;
            }

            if (selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // Graphics queue is selected at runtime, depending on target surfaces
                selection.graphics_families.push_back(family);
            }

            if (compute_score < 1 && (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                selection.compute_family = family;
                compute_score = 1;
            }

            if (compute_score < 2 && (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // Async compute queue is optimal
                selection.compute_family = family;
                compute_score = 2;
            }

            if (transfer_score < 1 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                selection.transfer_family = family;
                transfer_score = 1;
            }

            if (transfer_score < 2 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                selection.transfer_family = family;
                transfer_score = 2;
            }

            if (transfer_score < 3 && (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(selection.family_properties[family].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
                // Async transfer is optimal for host<->device transfers
                selection.transfer_family = family;
                transfer_score = 3;
            }

            HC_TRACE(
                "Queue family " << family << " properties: Count: " << std::setw(3)
                << selection.family_properties[family].queueCount << " Flags: "
                << (selection.family_properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "GRAPHICS | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_COMPUTE_BIT ? "COMPUTE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_TRANSFER_BIT ? "TRANSFER | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SPARSE_BINDING | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_PROTECTED_BIT ? "PROTECTED | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? "VIDEO_DECODE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ? "VIDEO_ENCODE | " : "")
                << (selection.family_properties[family].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ? "OPTICAL_FLOW | " : "")
                << (selection.family_properties[family].queueFlags ? "\b\b  " : "NONE")
            );
        }

        if (selection.graphics_families.empty()) {
            HC_ERROR("No graphics queue families found");
            return Error(HCError_UnmetQueueRequirements);
        }

#ifdef HC_LOGGING
        std::stringstream stream;
        stream << "Selected graphics queues: ";
        for (u32 family : selection.graphics_families) {
            stream << family << ", ";
        }
        stream << "\b\b  ";
        HC_DEBUG(stream.str());
#endif // HC_LOGGING

        if (selection.compute_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected compute queue family index: " << selection.compute_family);
        } else {
            HC_WARN("No compute queue family found");
            return Error(HCError_UnmetQueueRequirements);
        }

        if (selection.transfer_family != std::numeric_limits<u32>::max()) {
            HC_DEBUG("Selected transfer queue family index: " << selection.transfer_family);
        } else {
            HC_ERROR("No transfer queue family found");
            return Error(HCError_UnmetQueueRequirements);
        }

        return selection;
    }

    std::expected<Scheduler, Error> Scheduler::create(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        QueueSelection const& selection,
        u8 max_frames_in_flight
    ) {
        Scheduler scheduler;

        std::set<u32> unique_families;
        unique_families.insert(selection.graphics_families.begin(), selection.graphics_families.end());
        unique_families.insert(selection.compute_family);
        unique_families.insert(selection.transfer_family);

        for (u32 const queue_family : unique_families) {
            u32 const index = static_cast<u32>(scheduler.queues.size());

            auto found = std::ranges::find(selection.graphics_families, queue_family);
            if (found != selection.graphics_families.end()) {
                scheduler.graphics_queue_indexes.push_back(index);
            }

            if (selection.compute_family == queue_family) {
                scheduler.compute_queue_index = queue_family;
            }

            if (selection.transfer_family == queue_family) {
                scheduler.transfer_queue_index = queue_family;
            }

            VkQueue handle = VK_NULL_HANDLE;
            fn_table.vkGetDeviceQueue(device, queue_family, 0, &handle);

            std::vector<CommandPool> pools;
            pools.reserve(max_frames_in_flight);

            for (u8 i = 0; i < max_frames_in_flight; ++i) {
                auto pool_result = CommandPool::create(fn_table, device, queue_family);
                if (!pool_result) {
                    for (auto& pool : pools) {
                        pool.destroy(fn_table, device);
                    }
                    pools.clear();

                    scheduler.destroy(fn_table, device);

                    return std::unexpected(pool_result.error());
                }
                pools.push_back(*std::move(pool_result));
            }

            scheduler.queues.push_back(
                {
                    .handle = handle,
                    .family = queue_family,
                    .family_properties = selection.family_properties[queue_family],
                    .pools = std::move(pools),
                }
            );
        }

        for (u32 index : scheduler.graphics_queue_indexes) {
            scheduler.graphics_queue_refs.push_back(std::ref(scheduler.queues[index]));
        }

        return scheduler;
    }

    void Scheduler::destroy(VolkDeviceTable const& fn_table, VkDevice device) {
        for (Queue& queue : this->queues) {
            for (auto& pool : queue.pools) {
                pool.destroy(fn_table, device);
            }

            queue.pools.clear();

            // Queues aren't freed
            queue.handle = VK_NULL_HANDLE;
        }

        this->queues.clear();
    }

    std::optional<u32> Scheduler::present_support(
        VkPhysicalDevice const& physical_handle,
        VkSurfaceKHR const& surface
    ) const {
        for (u32 i = 0; i < this->graphics_queue_indexes.size(); i++) {
            u32 const family = this->queues[this->graphics_queue_indexes[i]].family;
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

    std::expected<void, Error> Scheduler::reset_pools(
        const VolkDeviceTable& fn_table,
        VkDevice device,
        u8 frame_mod
    ) {
        for (auto& queue : this->queues) {
            VkResult result = fn_table.vkResetCommandPool(device, queue.pools[frame_mod].handle, 0);

            if (result != VK_SUCCESS) {
                return Error(result);
            }
        }

        return {};
    }
}
